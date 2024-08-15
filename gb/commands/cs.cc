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
    if ((g.pnum = race.governor[Governor].defplanetnum) >=
        stars[g.snum].numplanets)
      g.pnum = stars[g.snum].numplanets - 1;
    g.shipno = 0;
    g.lastx[0] = g.lasty[0] = 0.0;
    g.lastx[1] = stars[g.snum].xpos;
    g.lasty[1] = stars[g.snum].ypos;
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
          g.lastx[1] = stars[g.snum].xpos;
          g.lasty[1] = stars[g.snum].ypos;
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto planet = getplanet(g.snum, g.pnum);
        if (where.level == ScopeLevel::LEVEL_STAR && where.snum == g.snum) {
          g.lastx[0] = planet.xpos;
          g.lasty[0] = planet.ypos;
        } else if (where.level == ScopeLevel::LEVEL_UNIV) {
          g.lastx[1] = stars[g.snum].xpos + planet.xpos;
          g.lasty[1] = stars[g.snum].ypos + planet.ypos;
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
      } break;
      case ScopeLevel::LEVEL_SHIP:
        auto s = getship(g.shipno);
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
                g.lastx[0] = s->xpos - stars[s->storbits].xpos;
                g.lasty[0] = s->ypos - stars[s->storbits].ypos;
              } else
                g.lastx[0] = g.lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_PLAN:
              if (s->whatorbits == ScopeLevel::LEVEL_PLAN &&
                  s->storbits == where.snum && s->pnumorbits == where.pnum) {
                /* same */
                const auto planet = getplanet(s->storbits, s->pnumorbits);
                g.lastx[0] = s->xpos - stars[s->storbits].xpos - planet.xpos;
                g.lasty[0] = s->ypos - stars[s->storbits].ypos - planet.ypos;
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

    race.governor[Governor].deflevel = where.level;
    race.governor[Governor].defsystem = where.snum;
    race.governor[Governor].defplanetnum = where.pnum;
    putrace(race);

    std::string where_str = where.to_string();
    g.out << "New home system is " << where_str << "\n";
    return;
  }
}
}  // namespace GB::commands
