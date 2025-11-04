// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void cs(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  auto &race = races[Playernum - 1];

  // Change to default scope
  if (argv.size() == 1) {
    g.level = race.governor[Governor].deflevel;
    if ((g.snum = race.governor[Governor].defsystem) >= Sdata.numstars)
      g.snum = Sdata.numstars - 1;
    const auto* star = g.entity_manager.peek_star(g.snum);
    if (star &&
        (g.pnum = race.governor[Governor].defplanetnum) >= star->numplanets)
      g.pnum = star->numplanets - 1;
    g.shipno = 0;
    g.lastx[0] = g.lasty[0] = 0.0;
    if (star) {
      g.lastx[1] = star->xpos;
      g.lasty[1] = star->ypos;
    }
    return;
  }

  // Change to specified scope
  if (argv.size() == 2) {
    Place where{g, argv[1]};

    if (where.err) {
      g.out << "cs: bad scope.\n";
      g.lastx[0] = g.lasty[0] = 0.0;
      return;
    }

    /* fix lastx, lasty coordinates */
    switch (g.level) {
      case ScopeLevel::LEVEL_UNIV:
        g.lastx[0] = g.lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_STAR:
        if (where.level == ScopeLevel::LEVEL_UNIV) {
          const auto* star = g.entity_manager.peek_star(g.snum);
          if (star) {
            g.lastx[1] = star->xpos;
            g.lasty[1] = star->ypos;
          }
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto* planet = g.entity_manager.peek_planet(g.snum, g.pnum);
        if (!planet) {
          g.lastx[0] = g.lasty[0] = 0.0;
          break;
        }
        const auto* star = g.entity_manager.peek_star(g.snum);
        if (where.level == ScopeLevel::LEVEL_STAR && where.snum == g.snum) {
          g.lastx[0] = planet->xpos;
          g.lasty[0] = planet->ypos;
        } else if (where.level == ScopeLevel::LEVEL_UNIV) {
          if (star) {
            g.lastx[1] = star->xpos + planet->xpos;
            g.lasty[1] = star->ypos + planet->ypos;
          }
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
      } break;
      case ScopeLevel::LEVEL_SHIP:
        const auto* s = g.entity_manager.peek_ship(g.shipno);
        if (!s) {
          g.lastx[0] = g.lasty[0] = 0.0;
          break;
        }
        if (!s->docked) {
          switch (where.level) {
            case ScopeLevel::LEVEL_UNIV:
              g.lastx[1] = s->xpos;
              g.lasty[1] = s->ypos;
              break;
            case ScopeLevel::LEVEL_STAR:
              if (s->whatorbits >= ScopeLevel::LEVEL_STAR &&
                  s->storbits == where.snum) {
                /* we are going UP from the ship.. change last*/
                const auto* orbit_star =
                    g.entity_manager.peek_star(s->storbits);
                if (orbit_star) {
                  g.lastx[0] = s->xpos - orbit_star->xpos;
                  g.lasty[0] = s->ypos - orbit_star->ypos;
                } else {
                  g.lastx[0] = g.lasty[0] = 0.0;
                }
              } else
                g.lastx[0] = g.lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_PLAN:
              if (s->whatorbits == ScopeLevel::LEVEL_PLAN &&
                  s->storbits == where.snum && s->pnumorbits == where.pnum) {
                /* same */
                const auto* planet =
                    g.entity_manager.peek_planet(s->storbits, s->pnumorbits);
                const auto* orbit_star =
                    g.entity_manager.peek_star(s->storbits);
                if (planet && orbit_star) {
                  g.lastx[0] = s->xpos - orbit_star->xpos - planet->xpos;
                  g.lasty[0] = s->ypos - orbit_star->ypos - planet->ypos;
                } else {
                  g.lastx[0] = g.lasty[0] = 0.0;
                }
              } else
                g.lastx[0] = g.lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_SHIP:
              g.lastx[0] = g.lasty[0] = 0.0;
              break;
          }
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
        break;
    }
    g.level = where.level;
    g.snum = where.snum;
    g.pnum = where.pnum;
    g.shipno = where.shipno;
    return;
  }

  if (argv.size() == 3 && argv[1] == "-d") {
    /* make new def scope */
    Place where{g, argv[2]};

    if (where.err || where.level == ScopeLevel::LEVEL_SHIP) {
      g.out << "cs: bad home system.\n";
      return;
    }

    auto race_handle = g.entity_manager.get_race(Playernum);
    race_handle->governor[Governor].deflevel = where.level;
    race_handle->governor[Governor].defsystem = where.snum;
    race_handle->governor[Governor].defplanetnum = where.pnum;

    std::string where_str = where.to_string();
    g.out << "New home system is " << where_str << "\n";
    return;
  }
}
}  // namespace GB::commands
