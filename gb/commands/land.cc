// SPDX-License-Identifier: Apache-2.0

/// \file land.cc
/// \brief Land a ship on a planet or friendly mothership.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Load a landed ship onto a friendly mothership in the same sector.
 */
bool load_landed_onto_mothership(GameObj& g, Ship& s, const Ship& mothership) {
  if (!mothership.is_landed()) {
    g.out << std::format("{} is not landed on a planet.\n", mothership);
    return false;
  }
  if (mothership.storbits() != s.storbits()) {
    g.out << "These ships are not in the same star system.\n";
    return false;
  }
  if (mothership.pnumorbits() != s.pnumorbits()) {
    g.out << "These ships are not landed on the same planet.\n";
    return false;
  }
  if (mothership.land_coords() != s.land_coords()) {
    g.out << "These ships are not in the same sector.\n";
    return false;
  }
  if (s.on()) {
    g.out << std::format("{} must be turned off before loading.\n", s);
    return false;
  }
  if (s.size() > mothership.hanger_space()) {
    g.out << std::format("Mothership does not have {} hanger space "
                         "available to load ship.\n",
                         s.size());
    return false;
  }

  auto dock_res =
      g.entity_manager.dock_carrier(s.number(), mothership.number());
  if (!dock_res) {
    if (dock_res.error() == DockError::CycleDetected) {
      g.out << "Cannot dock a ship into its own parasite craft.\n";
    }
    return false;
  }
  s.dock_into_carrier(mothership.number());
  g.out << std::format("{} loaded onto {} using 0 fuel.\n", s, mothership);
  return true;
}

/**
 * @brief Land a spaceborne ship into a friendly carrier in orbit.
 */
bool land_spaceborne_on_carrier(GameObj& g, Ship& s, const Ship& carrier) {
  if (s.whatorbits() != carrier.whatorbits()) {
    g.out << "Those ships are not in the same scope.\n";
    return false;
  }
  if (s.whatorbits() != ScopeLevel::LEVEL_PLAN &&
      s.whatorbits() != ScopeLevel::LEVEL_STAR) {
    g.out << "Ship is not in planet or star scope.\n";
    return false;
  }

  const double dist = carrier.coordinates().distance_to(s.coordinates());
  if (dist > DIST_TO_DOCK) {
    g.out << std::format("{} must be {} or closer to {}.\n", s, DIST_TO_DOCK,
                         carrier);
    return false;
  }

  const double fuel = DOCK_BASE_FUEL_COST +
                      dist * DOCK_DISTANCE_FUEL_FACTOR * std::sqrt(s.mass());
  if (s.fuel() < fuel) {
    g.out << "Not enough fuel.\n";
    return false;
  }
  if (s.size() > carrier.hanger_space()) {
    g.out << std::format("Mothership does not have {} hanger space "
                         "available to load ship.\n",
                         s.size());
    return false;
  }

  auto dock_res = g.entity_manager.dock_carrier(s.number(), carrier.number());
  if (!dock_res) {
    if (dock_res.error() == DockError::CycleDetected) {
      g.out << "Cannot dock a ship into its own parasite craft.\n";
    }
    return false;
  }
  s.consume_fuel(fuel);
  s.dock_into_carrier(carrier.number());
  g.out << std::format("{} landed on {} using {} fuel.\n", s, carrier, fuel);
  return true;
}

/**
 * @brief Land a friendly ship onto another ship or mothership.
 */
bool land_friendly(const command_t& argv, GameObj& g, Ship& s) {
  auto ship2tmp = string_to_shipnum(argv[2]);
  if (!ship2tmp) {
    g.out << std::format("Ship {} wasn't found.\n", argv[2]);
    return false;
  }

  const auto ship2no = *ship2tmp;

  try {
    return g.entity_manager.with_ship(ship2no, [&](const Ship& s2_check) {
      if (!g.check_commandable(s2_check)) {
        g.out << "Illegal format.\n";
        return false;
      }
      if (s2_check.type() == ShipType::OTYPE_FACTORY) {
        g.out << "Can't land on factories.\n";
        return false;
      }
      if (s.is_landed()) {
        return load_landed_onto_mothership(g, s, s2_check);
      }
      if (s.docked()) {
        g.out << std::format("{} is already docked or landed.\n", s);
        return false;
      }
      return land_spaceborne_on_carrier(g, s, s2_check);
    });
  } catch (const EntityNotFoundError&) {
    g.out << std::format("Ship #{} wasn't found.\n", ship2no);
    return false;
  }
}

/**
 * @brief Resolve planetary surface-to-orbit defensive gun fire against a
 * descending ship.
 */
void resolve_planetary_defense_fire(GameObj& g, Ship& s, const Star& star,
                                    Planet& p) {
  if (!DEFENSE) return;

  const player_t playernum = g.player();
  for (const Race& alien_race : RaceList::readonly(g.entity_manager)) {
    const player_t i = alien_race.Playernum;
    if (!s.alive() || i == playernum) continue;
    if (!p.info(i).popn || !p.info(i).guns || !p.info(i).destruct) continue;
    if (!alien_race.is_at_war_with(s.owner())) continue;

    g.entity_manager.mutate_race(i, [&](Race& alien) {
      const int strength = std::min(static_cast<int>(p.info(i).guns),
                                    static_cast<int>(p.info(i).destruct));
      if (strength <= 0) return;

      if (auto p2s_opt =
              shoot_planet_to_ship(g.entity_manager, alien, s, strength)) {
        const auto& [p_damage, p_short, p_long] = *p2s_opt;
        post(g.entity_manager, p_short, NewsType::COMBAT);
        notify_star(g.session_registry, g.entity_manager, 0, 0, s.storbits(),
                    p_short);
        warn_player(g.session_registry, g.entity_manager, i, star.governor(i),
                    p_long);
        g.session_registry.notify_player(s.owner(), s.governor(), p_long);
      }
      p.info(i).destruct -= strength;
    });
  }
}

/**
 * @brief Resolve ship crash impact, collateral sector destruction, and player
 * notifications.
 */
void handle_landing_crash(GameObj& g, Ship& s, const Star& star, Planet& p,
                          Coordinates target_coords, double fuel, int roll) {
  int numdest = 0;
  g.entity_manager.mutate_sectormap(
      s.storbits(), s.pnumorbits(), [&](SectorMap& smap) {
        auto result_opt = shoot_ship_to_planet(
            g.entity_manager, s, p,
            round_rand(static_cast<double>(s.destruct()) / 3.0), target_coords,
            smap, 0, guntype_t::HEAVY);
        numdest = result_opt ? result_opt->sectors_destroyed : 0;
      });

  const auto buf =
      std::format("BOOM!! {} crashes on sector {} with blast radius of {}.\n",
                  s, target_coords, numdest);
  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const player_t i = race.Playernum;
    if (p.info(i).numsectsowned || i == g.player()) {
      warn_player(g.session_registry, g.entity_manager, i, star.governor(i),
                  buf);
    }
  }

  if (roll) {
    g.out << std::format("Ship damage {}% (you rolled a {})\n", s.damage(),
                         roll);
  } else {
    g.out << std::format("You had {:.1f}f while the landing required {:.1f}f\n",
                         s.fuel(), fuel);
  }
  g.entity_manager.kill_ship(s.owner(), s);
}

/**
 * @brief Report sector terrain/ownership status and notify planetary neighbors
 * of touchdown.
 */
void report_landing_sector_status(GameObj& g, const Ship& s, const Star& star,
                                  const Planet& p, Coordinates target_coords) {
  const player_t playernum = g.player();
  g.entity_manager.with_sectormap(
      s.storbits(), s.pnumorbits(), [&](const SectorMap& smap) {
        const auto& sector = smap.get(target_coords);
        if (sector.is_wasted()) {
          g.out << "Warning: That sector is a wasteland!\n";
          return;
        }
        if (sector.get_owner() != 0 && sector.get_owner() != playernum) {
          g.entity_manager.with_race(
              sector.get_owner(), [&](const Race& alien) {
                if (g.race->is_allied_with(sector.get_owner()) &&
                    alien.is_allied_with(playernum)) {
                  g.out << std::format(
                      "You have landed on allied sector ({}).\n", alien.name);
                } else {
                  g.out << std::format(
                      "You have landed on an alien sector ({}).\n", alien.name);
                }
              });
        }
      });

  const auto landing_msg = std::format(
      "{} observed landing on sector {},planet /{}/{}.\n", s, s.land_coords(),
      star.get_name(), star.get_planet_name(s.pnumorbits()));
  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const player_t i = race.Playernum;
    if (p.info(i).numsectsowned && i != playernum) {
      g.session_registry.notify_player(i, star.governor(i), landing_msg);
    }
  }
  g.out << std::format("{} landed on planet.\n", s);
}

/**
 * @brief Lands a ship on a planet.
 */
bool land_planet(const command_t& argv, GameObj& g, Ship& s) {
  if (s.docked()) {
    g.out << std::format("{} is docked.\n", s);
    return false;
  }
  auto coords_opt = Coordinates::parse(argv[2]);
  if (!coords_opt) {
    g.out << "Invalid coordinates format. Use: x,y\n";
    return false;
  }
  const Coordinates target_coords = *coords_opt;
  if (s.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("{} doesn't orbit a planet.\n", s);
    return false;
  }
  if (!s.can_land()) {
    g.out << "This ship is not equipped to land.\n";
    return false;
  }
  if ((s.storbits() != g.snum()) || (s.pnumorbits() != g.pnum())) {
    g.out << "You have to cs to the planet it orbits.\n";
    return false;
  }
  if (!s.max_speed_capacity()) {
    g.out << "This ship is not rated for maneuvering.\n";
    return false;
  }

  const auto& star = *g.entity_manager.peek_star(s.storbits());

  bool ok = false;
  g.entity_manager.mutate_planet(s.storbits(), s.pnumorbits(), [&](Planet& p) {
    g.out << std::format("Planet /{}/{} has gravity field of {:.2f}.\n",
                         star.get_name(), star.get_planet_name(s.pnumorbits()),
                         p.gravity());

    const double dist =
        s.coordinates().distance_to(p.absolute_coordinates(star));
    g.out << std::format("Distance to planet: {:.2f}.\n", dist);

    if (dist > DIST_TO_LAND) {
      g.out << std::format(
          "{} must be {:.3g} or closer to the planet ({:.2f}).\n", s,
          DIST_TO_LAND, dist);
      return;
    }

    if (!p.is_valid(target_coords)) {
      g.out << "Illegal coordinates.\n";
      return;
    }

    if (!g.deduct_ap(s.storbits(), 1)) {
      g.out << "You don't have 1 action points there.\n";
      return;
    }

    ok = true;
    const double fuel = s.mass() * p.gravity() * LAND_GRAV_MASS_FACTOR;

    resolve_planetary_defense_fire(g, s, star, p);
    if (!s.alive()) {
      return;
    }

    if (auto [did_crash, roll] = s.roll_landing_crash(fuel); did_crash) {
      handle_landing_crash(g, s, star, p, target_coords, fuel, roll);
      return;
    }

    s.set_land_coords(target_coords);
    s.set_coordinates(p.absolute_coordinates(star));
    s.consume_fuel(fuel);
    s.land_on_planet();
    s.deststar() = s.storbits();
    s.destpnum() = s.pnumorbits();

    report_landing_sector_status(g, s, star, p, target_coords);
  });
  return ok;
}
}  // namespace

namespace GB::commands {

bool land(const command_t& argv, GameObj& g) {
  const governor_t governor = g.governor();
  bool any_landed = false;

  ShipList ships(g);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!GB::ship_matches_filter(argv[1], s)) continue;
    if (!s.is_authorized_for(governor)) continue;

    if (s.is_overloaded()) {
      g.out << std::format("{} is too overloaded to land.\n", s);
      continue;
    }
    if (s.type() == ShipType::OTYPE_QUARRY) {
      g.out << "You can't load quarries onto ship.\n";
      continue;
    }
    if (s.is_docked()) {
      g.out << "That ship is docked to another ship.\n";
      continue;
    }

    if (argv[2][0] == '#') {
      if (land_friendly(argv, g, s)) {
        any_landed = true;
      }
    } else {
      if (land_planet(argv, g, s)) {
        any_landed = true;
      }
    }
  }

  return any_landed;
}

const CommandDescriptor land_cmd{
    .name = "land",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "land <ship> <#mothership | x,y>",
    .description = "Land a ship onto a planet sector or mothership",
    .handler = &land,
};

}  // namespace GB::commands
