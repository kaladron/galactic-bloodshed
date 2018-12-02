// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "cs.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "getplace.h"
#include "races.h"
#include "ships.h"
#include "vars.h"

void center(const command_t &argv, const GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  auto where = Getplace(Playernum, Governor, argv[1], 1);

  if (where.err) {
    sprintf(buf, "cs: bad scope.\n");
    notify(Playernum, Governor, buf);
    return;
  } else if (where.level == ScopeLevel::LEVEL_SHIP) {
    notify(Playernum, Governor, "CHEATER!!!\n");
    return;
  }
  Dir[Playernum - 1][Governor].lastx[1] = Stars[where.snum]->xpos;
  Dir[Playernum - 1][Governor].lasty[1] = Stars[where.snum]->ypos;
}

void cs(const command_t &argv, const GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  shiptype *s;
  racetype *Race = races[Playernum - 1];

  if (argv.size() == 1) {
    /* chdir to def scope */
    Dir[Playernum - 1][Governor].level = Race->governor[Governor].deflevel;
    if ((Dir[Playernum - 1][Governor].snum =
             Race->governor[Governor].defsystem) >= Sdata.numstars)
      Dir[Playernum - 1][Governor].snum = Sdata.numstars - 1;
    if ((Dir[Playernum - 1][Governor].pnum =
             Race->governor[Governor].defplanetnum) >=
        Stars[Dir[Playernum - 1][Governor].snum]->numplanets)
      Dir[Playernum - 1][Governor].pnum =
          Stars[Dir[Playernum - 1][Governor].snum]->numplanets - 1;
    Dir[Playernum - 1][Governor].shipno = 0;
    Dir[Playernum - 1][Governor].lastx[0] =
        Dir[Playernum - 1][Governor].lasty[0] = 0.0;
    Dir[Playernum - 1][Governor].lastx[1] =
        Stars[Dir[Playernum - 1][Governor].snum]->xpos;
    Dir[Playernum - 1][Governor].lasty[1] =
        Stars[Dir[Playernum - 1][Governor].snum]->ypos;
    return;
  } else if (argv.size() == 2) {
    /* chdir to specified scope */

    auto where = Getplace(Playernum, Governor, args[1], 0);

    if (where.err) {
      sprintf(buf, "cs: bad scope.\n");
      notify(Playernum, Governor, buf);
      Dir[Playernum - 1][Governor].lastx[0] =
          Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      return;
    }

    /* fix lastx, lasty coordinates */

    switch (Dir[Playernum - 1][Governor].level) {
      case ScopeLevel::LEVEL_UNIV:
        Dir[Playernum - 1][Governor].lastx[0] =
            Dir[Playernum - 1][Governor].lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_STAR:
        if (where.level == ScopeLevel::LEVEL_UNIV) {
          Dir[Playernum - 1][Governor].lastx[1] =
              Stars[Dir[Playernum - 1][Governor].snum]->xpos;
          Dir[Playernum - 1][Governor].lasty[1] =
              Stars[Dir[Playernum - 1][Governor].snum]->ypos;
        } else
          Dir[Playernum - 1][Governor].lastx[0] =
              Dir[Playernum - 1][Governor].lasty[0] = 0.0;
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto &planet = getplanet(Dir[Playernum - 1][Governor].snum,
                                       Dir[Playernum - 1][Governor].pnum);
        if (where.level == ScopeLevel::LEVEL_STAR &&
            where.snum == Dir[Playernum - 1][Governor].snum) {
          Dir[Playernum - 1][Governor].lastx[0] = planet.xpos;
          Dir[Playernum - 1][Governor].lasty[0] = planet.ypos;
        } else if (where.level == ScopeLevel::LEVEL_UNIV) {
          Dir[Playernum - 1][Governor].lastx[1] =
              Stars[Dir[Playernum - 1][Governor].snum]->xpos + planet.xpos;
          Dir[Playernum - 1][Governor].lasty[1] =
              Stars[Dir[Playernum - 1][Governor].snum]->ypos + planet.ypos;
        } else
          Dir[Playernum - 1][Governor].lastx[0] =
              Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      } break;
      case ScopeLevel::LEVEL_SHIP:
        (void)getship(&s, Dir[Playernum - 1][Governor].shipno);
        if (!s->docked) {
          switch (where.level) {
            case ScopeLevel::LEVEL_UNIV:
              Dir[Playernum - 1][Governor].lastx[1] = s->xpos;
              Dir[Playernum - 1][Governor].lasty[1] = s->ypos;
              break;
            case ScopeLevel::LEVEL_STAR:
              if (s->whatorbits >= ScopeLevel::LEVEL_STAR &&
                  s->storbits == where.snum) {
                /* we are going UP from the ship.. change last*/
                Dir[Playernum - 1][Governor].lastx[0] =
                    s->xpos - Stars[s->storbits]->xpos;
                Dir[Playernum - 1][Governor].lasty[0] =
                    s->ypos - Stars[s->storbits]->ypos;
              } else
                Dir[Playernum - 1][Governor].lastx[0] =
                    Dir[Playernum - 1][Governor].lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_PLAN:
              if (s->whatorbits == ScopeLevel::LEVEL_PLAN &&
                  s->storbits == where.snum && s->pnumorbits == where.pnum) {
                /* same */
                const auto &planet =
                    getplanet((int)s->storbits, (int)s->pnumorbits);
                Dir[Playernum - 1][Governor].lastx[0] =
                    s->xpos - Stars[s->storbits]->xpos - planet.xpos;
                Dir[Playernum - 1][Governor].lasty[0] =
                    s->ypos - Stars[s->storbits]->ypos - planet.ypos;
              } else
                Dir[Playernum - 1][Governor].lastx[0] =
                    Dir[Playernum - 1][Governor].lasty[0] = 0.0;
              break;
            case ScopeLevel::LEVEL_SHIP:
              Dir[Playernum - 1][Governor].lastx[0] =
                  Dir[Playernum - 1][Governor].lasty[0] = 0.0;
              break;
          }
        } else
          Dir[Playernum - 1][Governor].lastx[0] =
              Dir[Playernum - 1][Governor].lasty[0] = 0.0;
        free(s);
        break;
    }
    Dir[Playernum - 1][Governor].level = where.level;
    Dir[Playernum - 1][Governor].snum = where.snum;
    Dir[Playernum - 1][Governor].pnum = where.pnum;
    Dir[Playernum - 1][Governor].shipno = where.shipno;
  } else if (argv.size() == 3 && argv[1][1] == 'd') {
    /* make new def scope */
    auto where = Getplace(Playernum, Governor, argv[2], 0);

    if (!where.err && where.level != ScopeLevel::LEVEL_SHIP) {
      Race->governor[Governor].deflevel = where.level;
      Race->governor[Governor].defsystem = where.snum;
      Race->governor[Governor].defplanetnum = where.pnum;
      putrace(Race);

      sprintf(buf, "New home system is %s\n", Dispplace(where).c_str());
    } else {
      sprintf(buf, "cs: bad home system.\n");
    }
    notify(Playernum, Governor, buf);
  }
}
