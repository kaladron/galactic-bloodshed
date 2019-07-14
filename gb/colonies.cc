// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/colonies.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tech.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

enum modes_t { COLONIES, PRODUCTION };

namespace {
void colonies_at_star(GameObj &g, racetype *Race, starnum_t star,
                      modes_t mode) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  planetnum_t i;
  int j;

  getstar(&(Stars[star]), star);
  if (!isset(Stars[star]->explored, Playernum)) return;

  for (i = 0; i < Stars[star]->numplanets; i++) {
    const auto pl = getplanet(star, i);

    if (pl.info[Playernum - 1].explored &&
        pl.info[Playernum - 1].numsectsowned &&
        (!Governor || Stars[star]->governor[Playernum - 1] == Governor)) {
      switch (mode) {
        case COLONIES:
          sprintf(
              buf,
              " %c %4.4s/%-4.4s%c%4d%3d%5d%8ld%3d%6lu%5d%6d "
              "%3d/%-3d%3.0f/%-3d%3d/%-3d",
              Psymbol[pl.type], Stars[star]->name, Stars[star]->pnames[i],
              (pl.info[Playernum - 1].autorep ? '*' : ' '),
              Stars[star]->governor[Playernum - 1],
              pl.info[Playernum - 1].numsectsowned,
              pl.info[Playernum - 1].tech_invest, pl.info[Playernum - 1].popn,
              pl.info[Playernum - 1].crystals, pl.info[Playernum - 1].resource,
              pl.info[Playernum - 1].destruct, pl.info[Playernum - 1].fuel,
              pl.info[Playernum - 1].tax, pl.info[Playernum - 1].newtax,
              compatibility(pl, Race), pl.conditions[TOXIC],
              pl.info[Playernum - 1].comread, pl.info[Playernum - 1].mob_set);
          notify(Playernum, Governor, buf);
          for (j = 1; j <= Num_races; j++)
            if ((j != Playernum) && (pl.info[j - 1].numsectsowned > 0)) {
              sprintf(buf, " %d", j);
              notify(Playernum, Governor, buf);
            }
          g.out << "\n";
          if (mode == 0) break;
          [[clang::fallthrough]]; /* Fall through if (mode == -1) */
        case PRODUCTION:
          sprintf(
              buf,
              " %c %4.4s/%-4.4s%c%3d%8.4f%8ld%3d%6d%5d%6d %6ld   %3d%8.2f\n",
              Psymbol[pl.type], Stars[star]->name, Stars[star]->pnames[i],
              (pl.info[Playernum - 1].autorep ? '*' : ' '),
              Stars[star]->governor[Playernum - 1],
              pl.info[Playernum - 1].prod_tech, pl.total_resources,
              pl.info[Playernum - 1].prod_crystals,
              pl.info[Playernum - 1].prod_res, pl.info[Playernum - 1].prod_dest,
              pl.info[Playernum - 1].prod_fuel,
              pl.info[Playernum - 1].prod_money,
              pl.info[Playernum - 1].tox_thresh,
              pl.info[Playernum - 1].est_production);
          notify(Playernum, Governor, buf);
          break;
      }
    }
  }
}
}  // namespace

void colonies(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int i;
  racetype *Race;
  placetype where;

  modes_t mode;

  if (argv[0] == "colonies")
    mode = COLONIES;
  else
    mode = PRODUCTION;

  switch (mode) {
    case COLONIES:
      notify(Playernum, Governor,
             "          ========== Colonization Report ==========\n");
      notify(Playernum, Governor,
             "  Planet     gov sec tech    popn  x   res  "
             "des  fuel  tax  cmpt/tox mob  Aliens\n");
      break;
    case PRODUCTION:
      notify(Playernum, Governor,
             "          ============ Production Report ==========\n");
      notify(Playernum, Governor,
             "  Planet     gov    tech deposit  x   res  "
             "des  fuel    tax   tox  est prod\n");
      break;
  }

  Race = races[Playernum - 1];
  getsdata(&Sdata);

  if (argv.size() < 2)
    for (starnum_t star = 0; star < Sdata.numstars; star++)
      colonies_at_star(g, Race, star, mode);
  else
    for (i = 1; i < argv.size(); i++) {
      where = getplace(g, argv[i], 0);
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "Bad location `%s'.\n", argv[i].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      colonies_at_star(g, Race, where.snum, mode);
    }
  g.out << "\n";
}
