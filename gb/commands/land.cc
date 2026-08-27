// SPDX-License-Identifier: Apache-2.0

/// \file land.cc
/// \brief Land a ship on a planet or friendly mothership.

module;

import session;
import gblib;
import notification;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Land a friendly ship onto another ship or planet.
 */
bool land_friendly(const command_t& argv, GameObj& g, Ship& s) {
  double fuel;
  double Dist;

  auto ship2tmp = string_to_shipnum(argv[2]);
  if (!ship2tmp) {
    g.out << std::format("Ship {} wasn't found.\n", argv[2]);
    return false;
  }

  const Ship* s2_check;
  try {
    s2_check = g.entity_manager.peek_ship(*ship2tmp);
  } catch (const EntityNotFoundError&) {
    g.out << std::format("Ship #{} wasn't found.\n", *ship2tmp);
    return false;
  }

  auto ship2no = *ship2tmp;
  if (testship(*s2_check, g)) {
    g.out << "Illegal format.\n";
    return false;
  }
  if (s2_check->type() == ShipType::OTYPE_FACTORY) {
    g.out << "Can't land on factories.\n";
    return false;
  }
  if (landed(s)) {
    if (!landed(*s2_check)) {
      g.out << std::format("{} is not landed on a planet.\n", *s2_check);
      return false;
    }
    if (s2_check->storbits() != s.storbits()) {
      g.out << "These ships are not in the same star system.\n";
      return false;
    }
    if (s2_check->pnumorbits() != s.pnumorbits()) {
      g.out << "These ships are not landed on the same planet.\n";
      return false;
    }
    if (s2_check->land_coords() != s.land_coords()) {
      g.out << "These ships are not in the same sector.\n";
      return false;
    }
    if (s.on()) {
      g.out << std::format("{} must be turned off before loading.\n", s);
      return false;
    }
    if (size(s) > hanger(*s2_check)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return false;
    }
    /* ok, load 'em up */
    remove_sh_plan(g.entity_manager, s);
    auto s2_handle = g.entity_manager.get_ship(ship2no);
    auto& s2 = *s2_handle;
    insert_sh_ship(&s, &s2);
    s2.mass() += s.mass();
    s2.hanger() += size(s);
    fuel = 0.0;
    g.out << std::format("{} loaded onto {} using {} fuel.\n", s, s2, fuel);
    s.docked() = 1;
    return true;
  } else if (s.docked()) {
    g.out << std::format("{} is already docked or landed.\n", s);
    return false;
  } else {
    if (s.whatorbits() != s2_check->whatorbits()) {
      g.out << "Those ships are not in the same scope.\n";
      return false;
    }

    Dist = std::hypot(s2_check->xpos() - s.xpos(), s2_check->ypos() - s.ypos());
    if (Dist > DIST_TO_DOCK) {
      g.out << std::format("{} must be {} or closer to {}.\n", s, DIST_TO_DOCK,
                           *s2_check);
      return false;
    }
    fuel = 0.05 + Dist * 0.025 * std::sqrt(s.mass());
    if (s.fuel() < fuel) {
      g.out << "Not enough fuel.\n";
      return false;
    }
    if (size(s) > hanger(*s2_check)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return false;
    }
    use_fuel(s, fuel);

    if (s.whatorbits() == ScopeLevel::LEVEL_PLAN)
      remove_sh_plan(g.entity_manager, s);
    else if (s.whatorbits() == ScopeLevel::LEVEL_STAR)
      remove_sh_star(g.entity_manager, s);
    else {
      g.out << "Ship is not in planet or star scope.\n";
      return false;
    }

    auto s2_handle = g.entity_manager.get_ship(ship2no);
    if (!s2_handle.get()) {
      g.out << "This shouldn't happen: Target ship no longer exists.\n";
      return false;
    }
    auto& s2 = *s2_handle;
    insert_sh_ship(&s, &s2);
    s2.mass() += s.mass();
    s2.hanger() += size(s);
    g.out << std::format("{} landed on {} using {} fuel.\n", s, s2, fuel);
    s.docked() = 1;
    return true;
  }
}

/**
 * @brief Lands a ship on a planet.
 */
bool land_planet(const command_t& argv, GameObj& g, Ship& s) {
  player_t Playernum = g.player();
  int numdest = 0;
  int strength;
  double fuel;
  double Dist;

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
  if (!Shipdata[s.type()][ABIL_CANLAND]) {
    g.out << "This ship is not equipped to land.\n";
    return false;
  }
  if ((s.storbits() != g.snum()) || (s.pnumorbits() != g.pnum())) {
    g.out << "You have to cs to the planet it orbits.\n";
    return false;
  }
  if (!speed_rating(s)) {
    g.out << "This ship is not rated for maneuvering.\n";
    return false;
  }

  const auto* star = g.entity_manager.peek_star(s.storbits());
  if (!star) {
    g.out << "Star system not found.\n";
    return false;
  }

  if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (!g.deduct_univ_ap(1)) {
      g.out << "You need 1 universe action point.\n";
      return false;
    }
  } else {
    if (!g.deduct_ap(s.storbits(), 1)) {
      g.out << "You don't have 1 action points there.\n";
      return false;
    }
  }

  auto planet_handle =
      g.entity_manager.get_planet(s.storbits(), s.pnumorbits());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return false;
  }
  auto& p = *planet_handle;

  g.out << std::format("Planet /{}/{} has gravity field of {:.2f}.\n",
                       star->get_name(), star->get_planet_name(s.pnumorbits()),
                       p.gravity());

  Dist = std::hypot((star->xpos() + p.xpos()) - s.xpos(),
                    (star->ypos() + p.ypos()) - s.ypos());
  g.out << std::format("Distance to planet: {:.2f}.\n", Dist);

  if (Dist > DIST_TO_LAND) {
    g.out << std::format(
        "{} must be {:.3g} or closer to the planet ({:.2f}).\n", s,
        DIST_TO_LAND, Dist);
    return false;
  }

  fuel = s.mass() * p.gravity() * LAND_GRAV_MASS_FACTOR;

  if (!p.is_valid(target_coords)) {
    g.out << "Illegal coordinates.\n";
    return false;
  }

  if (DEFENSE) {
    for (const Race& alien_race : RaceList::readonly(g.entity_manager)) {
      const auto i = alien_race.Playernum;
      if (s.alive() && i != Playernum && p.info(i).popn && p.info(i).guns &&
          p.info(i).destruct) {
        if (isset(alien_race.atwar, s.owner())) {
          auto alien_handle = g.entity_manager.get_race(i);
          if (!alien_handle.get()) continue;
          auto& alien = *alien_handle;
          strength = MIN((int)p.info(i).guns, (int)p.info(i).destruct);
          if (strength) {
            if (auto p2s_opt = shoot_planet_to_ship(g.entity_manager, alien, s,
                                                    strength)) {
              auto [p_damage, p_short, p_long] = *p2s_opt;
              post(g.entity_manager, p_short, NewsType::COMBAT);
              notify_star(g.session_registry, g.entity_manager, 0, 0,
                          s.storbits(), p_short);
              warn_player(g.session_registry, g.entity_manager, i,
                          star->governor(i), p_long);
              g.session_registry.notify_player(s.owner(), s.governor(), p_long);
            }
            p.info(i).destruct -= strength;
          }
        }
      }
    }
    if (!s.alive()) {
      return false;
    }
  }

  if (auto [did_crash, roll] = crash(s, fuel); did_crash) {
    auto smap_handle =
        g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
    auto& smap = *smap_handle;
    auto result_opt = shoot_ship_to_planet(
        g.entity_manager, s, p, round_rand((double)(s.destruct()) / 3.),
        target_coords, smap, 0, GTYPE_HEAVY);
    numdest = result_opt ? std::get<0>(*result_opt) : 0;
    auto buf =
        std::format("BOOM!! {} crashes on sector {} with blast radius of {}.\n",
                    s, target_coords, numdest);
    for (const Race& race : RaceList::readonly(g.entity_manager)) {
      const auto i = race.Playernum;
      if (p.info(i).numsectsowned || i == Playernum)
        warn_player(g.session_registry, g.entity_manager, i, star->governor(i),
                    buf);
    }
    if (roll)
      g.out << std::format("Ship damage {}% (you rolled a {})\n",
                           (int)s.damage(), roll);
    else
      g.out << std::format(
          "You had {:.1f}f while the landing required {:.1f}f\n", s.fuel(),
          fuel);
    g.entity_manager.kill_ship(s.owner(), s);
  } else {
    auto smap_handle =
        g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());

    s.set_land_coords(target_coords);
    s.xpos() = p.xpos() + star->xpos();
    s.ypos() = p.ypos() + star->ypos();
    use_fuel(s, fuel);
    s.docked() = 1;
    s.whatdest() = ScopeLevel::LEVEL_PLAN;
    s.deststar() = s.storbits();
    s.destpnum() = s.pnumorbits();
  }

  auto smap_handle =
      g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
  auto& smap = *smap_handle;
  auto& sector = smap.get(target_coords);

  if (sector.is_wasted()) {
    g.out << "Warning: That sector is a wasteland!\n";
  } else if (sector.get_owner() != 0 && sector.get_owner() != Playernum) {
    const auto* alien = g.entity_manager.peek_race(sector.get_owner());
    if (alien) {
      if (!(isset(g.race->allied, sector.get_owner()) &&
            isset(alien->allied, Playernum))) {
        g.out << std::format("You have landed on an alien sector ({}).\n",
                             alien->name);
      } else {
        g.out << std::format("You have landed on allied sector ({}).\n",
                             alien->name);
      }
    }
  }

  auto landing_msg = std::format(
      "{} observed landing on sector {},planet /{}/{}.\n", s, s.land_coords(),
      star->get_name(), star->get_planet_name(s.pnumorbits()));
  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const auto i = race.Playernum;
    if (p.info(i).numsectsowned && i != Playernum) {
      g.session_registry.notify_player(i, star->governor(i), landing_msg);
    }
  }
  g.out << std::format("{} landed on planet.\n", s);
  return true;
}
}  // namespace

namespace GB::commands {

bool land(const command_t& argv, GameObj& g) {
  governor_t Governor = g.governor();
  bool any_landed = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!GB::ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (overloaded(s)) {
      g.out << std::format("{} is too overloaded to land.\n", s);
      continue;
    }
    if (s.type() == ShipType::OTYPE_QUARRY) {
      g.out << "You can't load quarries onto ship.\n";
      continue;
    }
    if (docked(s)) {
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
