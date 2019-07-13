// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/cs.h"

#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

void center(const command_t &argv, GameObj &g) {
  if (argv.size() != 2) {
    g.out << "center: which star?\n";
  }
  auto where = getplace(g, argv[1], 1);

  if (where.err) {
    g.out << "center: bad scope.\n";
    return;
  }
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    g.out << "CHEATER!!!\n";
    return;
  }
  g.lastx[1] = Stars[where.snum]->xpos;
  g.lasty[1] = Stars[where.snum]->ypos;
}

void cs(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  racetype *Race = races[Playernum - 1];

  if (argv.size() == 1) {
    /* chdir to def scope */
    g.level = Race->governor[Governor].deflevel;
    if ((g.snum = Race->governor[Governor].defsystem) >= Sdata.numstars)
      g.snum = Sdata.numstars - 1;
    if ((g.pnum = Race->governor[Governor].defplanetnum) >=
        Stars[g.snum]->numplanets)
      g.pnum = Stars[g.snum]->numplanets - 1;
    g.shipno = 0;
    g.lastx[0] = g.lasty[0] = 0.0;
    g.lastx[1] = Stars[g.snum]->xpos;
    g.lasty[1] = Stars[g.snum]->ypos;
    return;
  }
  if (argv.size() == 2) {
    /* chdir to specified scope */

    auto where = getplace(g, argv[1], 0);

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
          g.lastx[1] = Stars[g.snum]->xpos;
          g.lasty[1] = Stars[g.snum]->ypos;
        } else
          g.lastx[0] = g.lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto planet = getplanet(g.snum, g.pnum);
        if (where.level == ScopeLevel::LEVEL_STAR && where.snum == g.snum) {
          g.lastx[0] = planet.xpos;
          g.lasty[0] = planet.ypos;
        } else if (where.level == ScopeLevel::LEVEL_UNIV) {
          g.lastx[1] = Stars[g.snum]->xpos + planet.xpos;
          g.lasty[1] = Stars[g.snum]->ypos + planet.ypos;
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
                g.lastx[0] = s->xpos - Stars[s->storbits]->xpos;
                g.lasty[0] = s->ypos - Stars[s->storbits]->ypos;
              } else
                g.lastx[0] = g.lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_PLAN:
              if (s->whatorbits == ScopeLevel::LEVEL_PLAN &&
                  s->storbits == where.snum && s->pnumorbits == where.pnum) {
                /* same */
                const auto planet = getplanet(s->storbits, s->pnumorbits);
                g.lastx[0] = s->xpos - Stars[s->storbits]->xpos - planet.xpos;
                g.lasty[0] = s->ypos - Stars[s->storbits]->ypos - planet.ypos;
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
  } else if (argv.size() == 3 && argv[1][1] == 'd') {
    /* make new def scope */
    auto where = getplace(g, argv[2], 0);

    if (!where.err && where.level != ScopeLevel::LEVEL_SHIP) {
      Race->governor[Governor].deflevel = where.level;
      Race->governor[Governor].defsystem = where.snum;
      Race->governor[Governor].defplanetnum = where.pnum;
      putrace(Race);

      g.out << "New home system is " << Dispplace(where) << "\n";
    } else {
      g.out << "cs: bad home system.\n";
    }
  }
}