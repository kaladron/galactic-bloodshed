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

namespace GB::commands {

bool launch(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  bool any_launched = false;

  ShipList ships(g);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (!speed_rating(s) && landed(s)) {
      g.out << "That ship is not designed to be launched.\n";
      continue;
    }

    if (!s.docked() && s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
      g.out << std::format("{} is not landed or docked.\n", s);
      continue;
    }
    if (landed(s) && s.resource() > max_resource(s)) {
      g.out << std::format("{} is too overloaded to launch.\n", s);
      continue;
    }
    if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      if (s.type() == ShipType::OTYPE_FACTORY && s.on()) {
        g.out << "Factories cannot be launched once turned on.\n";
        g.out << "Consider using 'scrap'.\n";
        continue;
      }
      g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
        if (landed(s2)) {
          g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
            s.whatorbits() = ScopeLevel::LEVEL_PLAN;
            s.storbits() = s2.storbits();
            s.pnumorbits() = s2.pnumorbits();
            s.destpnum() = s2.pnumorbits();
            s.deststar() = s2.deststar();
            s.destshipno() = 0;
            s.xpos() = s2.xpos();
            s.ypos() = s2.ypos();
            s.set_land_coords(s2.land_coords());
            s.docked() = 1;
            s.whatdest() = ScopeLevel::LEVEL_PLAN;
            s2.mass() -= s.mass();
            s2.hanger() -= size(s);
            g.out << std::format("Landed on {}/{}.\n", star.get_name(),
                                 star.get_planet_name(s.pnumorbits()));
          });
        } else if (s2.whatorbits() == ScopeLevel::LEVEL_PLAN) {
          g.out << std::format("{} launched from {}.\n", s, s2);
          s.whatorbits() = ScopeLevel::LEVEL_PLAN;
          s.storbits() = s2.storbits();
          s.pnumorbits() = s2.pnumorbits();
          s.destshipno() = 0;
          s.xpos() = s2.xpos();
          s.ypos() = s2.ypos();
          s.docked() = 0;
          s.whatdest() = ScopeLevel::LEVEL_UNIV;
          s2.mass() -= s.mass();
          s2.hanger() -= size(s);
          g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
            g.out << std::format("Orbiting {}/{}.\n", star.get_name(),
                                 star.get_planet_name(s.pnumorbits()));
          });
        } else if (s2.whatorbits() == ScopeLevel::LEVEL_STAR) {
          g.out << std::format("{} launched from {}.\n", s, s2);
          s.whatorbits() = ScopeLevel::LEVEL_STAR;
          s.storbits() = s2.storbits();
          s.destshipno() = 0;
          s.xpos() = s2.xpos();
          s.ypos() = s2.ypos();
          s.docked() = 0;
          s.whatdest() = ScopeLevel::LEVEL_UNIV;
          s2.mass() -= s.mass();
          s2.hanger() -= size(s);
          g.entity_manager.with_star(s2.storbits(), [&](const Star& star) {
            g.out << std::format("Orbiting {}.\n", star.get_name());
          });
        } else if (s2.whatorbits() == ScopeLevel::LEVEL_UNIV) {
          g.out << std::format("{} launched from {}.\n", s, s2);
          s.whatorbits() = ScopeLevel::LEVEL_UNIV;
          s.destshipno() = 0;
          s.xpos() = s2.xpos();
          s.ypos() = s2.ypos();
          s.docked() = 0;
          s.whatdest() = ScopeLevel::LEVEL_UNIV;
          s2.mass() -= s.mass();
          s2.hanger() -= size(s);
          g.out << "Universe level.\n";
        } else {
          g.out << "You can't launch that ship.\n";
          return;
        }
        any_launched = true;
      });
    } else if (s.whatdest() == ScopeLevel::LEVEL_SHIP) {
      g.entity_manager.mutate_ship(s.destshipno(), [&](Ship& s2) {
        if (s2.whatorbits() == ScopeLevel::LEVEL_UNIV) {
          if (!g.deduct_univ_ap(1)) {
            g.out << "You need 1 universe action point.\n";
            return;
          }
        } else {
          if (!g.deduct_ap(s.storbits(), 1)) {
            g.out << "You don't have 1 action points there.\n";
            return;
          }
        }
        s.docked() = 0;
        s.whatdest() = ScopeLevel::LEVEL_UNIV;
        s.destshipno() = 0;
        s2.docked() = 0;
        s2.whatdest() = ScopeLevel::LEVEL_UNIV;
        s2.destshipno() = 0;
        g.out << std::format("{} undocked from {}.\n", s, s2);
        any_launched = true;
      });
    } else {
      if (!g.deduct_ap(s.storbits(), 1)) {
        g.out << "You don't have 1 action points there.\n";
        return any_launched;
      }

      g.entity_manager.with_star(s.storbits(), [&](const Star& star) {
        g.entity_manager.mutate_planet(
            s.storbits(), s.pnumorbits(), [&](Planet& p) {
              g.out << std::format(
                  "Planet /{}/{} has gravity field of {:.2f}\n",
                  star.get_name(), star.get_planet_name(s.pnumorbits()),
                  p.gravity());
              s.xpos() = star.xpos() + p.xpos() +
                         (double)int_rand((int)(-DIST_TO_LAND / 4),
                                          (int)(DIST_TO_LAND / 4));
              s.ypos() = star.ypos() + p.ypos() +
                         (double)int_rand((int)(-DIST_TO_LAND / 4),
                                          (int)(DIST_TO_LAND / 4));

              auto fuel = p.gravity() * s.mass() * LAUNCH_GRAV_MASS_FACTOR;
              if (s.fuel() < fuel) {
                g.out << std::format("{} does not have enough fuel! ({:.1f})\n",
                                     s, fuel);
                return;
              }
              use_fuel(s, fuel);
              s.docked() = 0;
              s.whatdest() = ScopeLevel::LEVEL_UNIV;
              switch (s.type()) {
                case ShipType::OTYPE_CANIST:
                case ShipType::OTYPE_GREEN:
                  s.special() = TimerData{.count = 0};
                  break;
                default:
                  break;
              }
              s.notified() = 0;
              if (!p.explored()) {
                p.explored() = 1;
              }
              std::string observed = std::format(
                  "{} observed launching from planet /{}/{}.\n", s,
                  star.get_name(), star.get_planet_name(s.pnumorbits()));
              for (player_t i = 1; i <= g.entity_manager.num_races(); i++)
                if (p.info(i).numsectsowned && i != Playernum) {
                  g.session_registry.notify_player(i, star.governor(i),
                                                   observed);
                }

              g.out << std::format("{} launched from planet,", s);
              g.out << std::format(" using {:.1f} fuel.\n", fuel);

              switch (s.type()) {
                case ShipType::OTYPE_CANIST:
                  g.out << "A cloud of dust envelopes your planet.\n";
                  break;
                case ShipType::OTYPE_GREEN:
                  g.out << "Greenhouse gases surround the planet.\n";
                  break;
                default:
                  break;
              }
              any_launched = true;
            });
      });
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
