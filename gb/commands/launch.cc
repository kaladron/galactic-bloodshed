// SPDX-License-Identifier: Apache-2.0

/// \file launch.cc
/// \brief Launch a landed or docked ship.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import std;

module commands;

namespace {

/**
 * @brief Deploy a carried ship from a landed mothership onto the same planetary
 * sector, or launch it into the carrier's current orbital scope.
 */
bool launch_from_carrier(GameObj& g, Ship& s) {
  if (s.type() == ShipType::OTYPE_FACTORY && s.on()) {
    g.out << "Factories cannot be launched once turned on.\n";
    g.out << "Consider using 'scrap'.\n";
    return false;
  }

  bool launched = false;
  g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
    if (s2.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      g.out << std::format(
          "{}'s mothership is currently berthed inside another vessel; "
          "launch {} first.\n",
          s, s2);
      return;
    }

    if (s2.is_landed()) {
      g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
        s.whatorbits() = ScopeLevel::LEVEL_PLAN;
        s.storbits() = s2.storbits();
        s.land_on_planet();
        s.pnumorbits() = s2.pnumorbits();
        s.destpnum() = s2.pnumorbits();
        s.deststar() = s2.deststar();
        s.destshipno() = 0;
        s.set_coordinates(s2.coordinates());
        s.set_land_coords(s2.land_coords());
        s2.set_mass(s2.mass() - s.mass());
        s2.hanger() -= s.size();
        g.out << std::format("Landed on {}/{}.\n", star.get_name(),
                             star.get_planet_name(s.pnumorbits()));
      });
      launched = true;
      return;
    }

    if (s2.whatorbits() != ScopeLevel::LEVEL_PLAN &&
        s2.whatorbits() != ScopeLevel::LEVEL_STAR &&
        s2.whatorbits() != ScopeLevel::LEVEL_UNIV) {
      g.out << "You can't launch that ship.\n";
      return;
    }

    g.out << std::format("{} launched from {}.\n", s, s2);
    s.launch_to_orbit(s2.whatorbits());
    s.set_coordinates(s2.coordinates());
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    s2.set_mass(s2.mass() - s.mass());
    s2.hanger() -= s.size();

    if (s2.whatorbits() == ScopeLevel::LEVEL_PLAN) {
      s.storbits() = s2.storbits();
      s.pnumorbits() = s2.pnumorbits();
      g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
        g.out << std::format("Orbiting {}/{}.\n", star.get_name(),
                             star.get_planet_name(s.pnumorbits()));
      });
    } else if (s2.whatorbits() == ScopeLevel::LEVEL_STAR) {
      s.storbits() = s2.storbits();
      g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
        g.out << std::format("Orbiting {}.\n", star.get_name());
      });
    } else {
      g.out << "Universe level.\n";
    }
    launched = true;
  });

  return launched;
}

/**
 * @brief Undock a ship that is moored ship-to-ship.
 */
bool undock_moored_ship(GameObj& g, Ship& s) {
  const auto s2_no = s.destshipno();
  const auto* s2_peek = g.entity_manager.peek_ship(s2_no);
  if (!s2_peek) {
    g.out << "Target ship not found.\n";
    return false;
  }

  if (s2_peek->whatorbits() == ScopeLevel::LEVEL_UNIV) {
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

  const auto s2_str = std::format("{}", *s2_peek);
  if (auto res = g.entity_manager.unmoor_ships(s.number()); !res) {
    g.out << "Failed to unmoor ship.\n";
    return false;
  }
  g.out << std::format("{} undocked from {}.\n", s, s2_str);
  return true;
}

/**
 * @brief Launch a landed ship from a planetary surface into low orbit.
 */
bool launch_from_planet(GameObj& g, Ship& s) {
  const player_t playernum = g.player();
  bool launched = false;

  g.entity_manager.with_star(s.storbits(), [&](const Star& star) {
    g.entity_manager.mutate_planet(
        s.storbits(), s.pnumorbits(), [&](Planet& p) {
          g.out << std::format(
              "Planet /{}/{} has gravity field of {:.2f}\n", star.get_name(),
              star.get_planet_name(s.pnumorbits()), p.gravity());

          const double fuel = p.gravity() * s.mass() * LAUNCH_GRAV_MASS_FACTOR;
          if (s.fuel() < fuel) {
            g.out << std::format("{} does not have enough fuel! ({:.1f})\n", s,
                                 fuel);
            return;
          }

          if (!g.deduct_ap(s.storbits(), 1)) {
            g.out << "You don't have 1 action points there.\n";
            return;
          }

          const double r_x =
              static_cast<double>(int_rand(static_cast<int>(-DIST_TO_LAND / 4),
                                           static_cast<int>(DIST_TO_LAND / 4)));
          const double r_y =
              static_cast<double>(int_rand(static_cast<int>(-DIST_TO_LAND / 4),
                                           static_cast<int>(DIST_TO_LAND / 4)));
          s.set_coordinates(p.absolute_coordinates(star) +
                            SystemCoordinates{r_x, r_y});

          s.consume_fuel(fuel);
          s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
          s.whatdest() = ScopeLevel::LEVEL_UNIV;
          if (auto* canist = s.as<CanisterShip>()) {
            canist->reset_timer();
          }
          s.notified() = 0;
          if (!p.explored()) {
            p.explored() = 1;
          }

          const std::string observed = std::format(
              "{} observed launching from planet /{}/{}.\n", s, star.get_name(),
              star.get_planet_name(s.pnumorbits()));
          for (const Race& race : RaceList::readonly(g.entity_manager)) {
            const player_t i = race.Playernum;
            if (p.info(i).numsectsowned && i != playernum) {
              g.session_registry.notify_player(i, star.governor(i), observed);
            }
          }

          g.out << std::format("{} launched from planet, using {:.1f} fuel.\n",
                               s, fuel);

          if (const auto* canist = s.as<CanisterShip>()) {
            if (canist->type() == ShipType::OTYPE_CANIST) {
              g.out << "A cloud of dust envelopes your planet.\n";
            } else if (canist->type() == ShipType::OTYPE_GREEN) {
              g.out << "Greenhouse gases surround the planet.\n";
            }
          }
          launched = true;
        });
  });

  return launched;
}

}  // namespace

namespace GB::commands {

bool launch(const command_t& argv, GameObj& g) {
  const governor_t governor = g.governor();
  bool any_launched = false;

  ShipList ships(g);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!s.is_authorized_for(governor)) continue;

    if (!s.max_speed_capacity() && s.is_landed()) {
      g.out << "That ship is not designed to be launched.\n";
      continue;
    }

    if (!s.docked() && s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
      g.out << std::format("{} is not landed or docked.\n", s);
      continue;
    }
    if (s.is_landed() && s.resource() > s.max_resource_capacity()) {
      g.out << std::format("{} is too overloaded to launch.\n", s);
      continue;
    }

    if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      if (launch_from_carrier(g, s)) {
        any_launched = true;
      }
    } else if (s.whatdest() == ScopeLevel::LEVEL_SHIP) {
      if (undock_moored_ship(g, s)) {
        any_launched = true;
      }
    } else {
      if (launch_from_planet(g, s)) {
        any_launched = true;
      }
    }
  }

  return any_launched;
}

const std::array<std::string_view, 1> launch_aliases = {"undock"};

const CommandDescriptor launch_cmd{
    .name = "launch",
    .aliases = launch_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "launch <ship>",
    .description = "Launch a landed or docked ship",
    .handler = &launch,
};

}  // namespace GB::commands
