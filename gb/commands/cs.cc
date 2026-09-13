// SPDX-License-Identifier: Apache-2.0

/// \file cs.cc
/// \brief Change current scope level or default home system.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool cs(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();

  // Change to default scope
  if (argv.size() == 1) {
    const auto* universe = g.entity_manager.peek_universe();
    if (!universe) {
      g.out << "cs: Universe data not available.\n";
      return false;
    }

    g.set_level(g.race->governor[Governor.value].deflevel);
    g.set_snum(g.race->governor[Governor.value].defsystem);
    if (g.snum() >= universe->numstars) g.set_snum(universe->numstars - 1);
    const auto& star = *g.entity_manager.peek_star(g.snum());
    g.set_pnum(g.race->governor[Governor.value].defplanetnum);
    if (g.pnum() >= star.numplanets()) g.set_pnum(star.numplanets() - 1);
    g.set_shipno(0);
    g.set_system_center({0.0, 0.0});
    g.set_universe_center(star.coordinates());
    return true;
  }

  // Change to specified scope
  if (argv.size() == 2) {
    Place where{g, argv[1]};

    if (where.err) {
      g.out << "cs: bad scope.\n";
      g.set_system_center({0.0, 0.0});
      return false;
    }

    /* fix viewport center coordinates */
    switch (g.level()) {
      case ScopeLevel::LEVEL_UNIV:
        g.set_system_center({0.0, 0.0});
        break;
      case ScopeLevel::LEVEL_STAR:
        if (where.level == ScopeLevel::LEVEL_UNIV) {
          const auto* star = g.entity_manager.peek_star(g.snum());
          if (star) {
            g.set_universe_center(star->coordinates());
          }
        } else {
          g.set_system_center({0.0, 0.0});
        }
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto* planet = g.entity_manager.peek_planet(g.snum(), g.pnum());
        if (!planet) {
          g.set_system_center({0.0, 0.0});
          break;
        }
        const auto* star = g.entity_manager.peek_star(g.snum());
        if (where.level == ScopeLevel::LEVEL_STAR && where.snum == g.snum()) {
          g.set_system_center(planet->system_coordinates());
        } else if (where.level == ScopeLevel::LEVEL_UNIV) {
          if (star) {
            g.set_universe_center(planet->absolute_coordinates(*star));
          }
        } else {
          g.set_system_center({0.0, 0.0});
        }
      } break;
      case ScopeLevel::LEVEL_SHIP: {
        const auto* s = g.entity_manager.peek_ship(g.shipno());
        if (!s) {
          g.set_system_center({0.0, 0.0});
          break;
        }
        if (!s->docked()) {
          switch (where.level) {
            case ScopeLevel::LEVEL_UNIV:
              g.set_universe_center(s->coordinates());
              break;
            case ScopeLevel::LEVEL_STAR:
              if (s->whatorbits() >= ScopeLevel::LEVEL_STAR &&
                  s->storbits() == where.snum) {
                /* we are going UP from the ship.. change system center */
                const auto* orbit_star =
                    g.entity_manager.peek_star(s->storbits());
                if (orbit_star) {
                  g.set_system_center(s->coordinates() -
                                      orbit_star->coordinates());
                } else {
                  g.set_system_center({0.0, 0.0});
                }
              } else {
                g.set_system_center({0.0, 0.0});
              }
              break;
            case ScopeLevel::LEVEL_PLAN:
              if (s->whatorbits() == ScopeLevel::LEVEL_PLAN &&
                  s->storbits() == where.snum &&
                  s->pnumorbits() == where.pnum) {
                const auto* planet = g.entity_manager.peek_planet(
                    s->storbits(), s->pnumorbits());
                const auto* orbit_star =
                    g.entity_manager.peek_star(s->storbits());
                if (planet && orbit_star) {
                  const auto pl_coords =
                      planet->absolute_coordinates(*orbit_star);
                  g.set_system_center(s->coordinates() - pl_coords);
                } else {
                  g.set_system_center({0.0, 0.0});
                }
              } else {
                g.set_system_center({0.0, 0.0});
              }
              break;
            case ScopeLevel::LEVEL_SHIP:
              g.set_system_center({0.0, 0.0});
              break;
          }
        } else {
          g.set_system_center({0.0, 0.0});
        }
      } break;
    }
    g.set_level(where.level);
    g.set_snum(where.snum);
    g.set_pnum(where.pnum);
    g.set_shipno(where.shipno);
    return true;
  }

  if (argv.size() == 3 && argv[1] == "-d") {
    Place where{g, argv[2]};

    if (where.err || where.level == ScopeLevel::LEVEL_SHIP) {
      g.out << "cs: bad home system.\n";
      return false;
    }

    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      race.governor[Governor.value].deflevel = where.level;
      race.governor[Governor.value].defsystem = where.snum;
      race.governor[Governor.value].defplanetnum = where.pnum;
    });

    std::string where_str = where.to_string();
    g.out << "New home system is " << where_str << "\n";
    return true;
  }

  g.out << "cs: bad scope.\n";
  return false;
}

const CommandDescriptor cs_cmd{
    .name = "cs",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "cs [<scope> | -d <scope>]",
    .description = "Change current scope level or default home system",
    .handler = &cs,
};

}  // namespace GB::commands
