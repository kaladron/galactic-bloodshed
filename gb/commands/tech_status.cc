// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/tech_status.h"

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

namespace {
void tech_report_star(int Playernum, int Governor, startype *star,
                      starnum_t snum, int *t_invest, double *t_gain,
                      double *t_max_gain) {
  char str[200];
  double gain;
  double max_gain;

  if (isset(star->explored, Playernum) &&
      (!Governor || star->governor[Playernum - 1] == Governor)) {
    for (planetnum_t i = 0; i < star->numplanets; i++) {
      const auto pl = getplanet(snum, i);
      if (pl.info[Playernum - 1].explored &&
          pl.info[Playernum - 1].numsectsowned) {
        sprintf(str, "%s/%s%s", star->name, star->pnames[i],
                (pl.info[Playernum - 1].autorep ? "*" : ""));
        sprintf(buf, "%16.16s %10ld%10d%8.3lf%8.3lf\n", str,
                pl.info[Playernum - 1].popn, pl.info[Playernum - 1].tech_invest,
                gain = tech_prod((int)pl.info[Playernum - 1].tech_invest,
                                 (int)pl.info[Playernum - 1].popn),
                max_gain = tech_prod((int)pl.info[Playernum - 1].prod_res,
                                     (int)pl.info[Playernum - 1].popn));
        notify(Playernum, Governor, buf);
        *t_invest += pl.info[Playernum - 1].tech_invest;
        *t_gain += gain;
        *t_max_gain += max_gain;
      }
    }
  }
}
}  // namespace

void tech_status(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int k;
  double total_gain = 0.0;
  double total_max_gain = 0.0;
  int total_invest = 0;

  getsdata(&Sdata);

  sprintf(buf, "             ========== Technology Report ==========\n\n");
  notify(Playernum, Governor, buf);

  sprintf(buf, "       Planet          popn    invest    gain   ^gain\n");
  notify(Playernum, Governor, buf);

  if (argv.size() == 1) {
    for (starnum_t star = 0; star < Sdata.numstars; star++) {
      getstar(&(Stars[star]), star);
      tech_report_star(Playernum, Governor, Stars[star], star, &total_invest,
                       &total_gain, &total_max_gain);
    }
  } else { /* Several arguments */
    for (k = 1; k < argv.size(); k++) {
      Place where{g, argv[k]};
      if (where.err || where.level == ScopeLevel::LEVEL_UNIV ||
          where.level == ScopeLevel::LEVEL_SHIP) {
        sprintf(buf, "Bad location `%s'.\n", argv[k].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      starnum_t star = where.snum;
      getstar(&Stars[star], star);
      tech_report_star(Playernum, Governor, Stars[star], star, &total_invest,
                       &total_gain, &total_max_gain);
    }
  }
  sprintf(buf, "       Total Popn:  %7ld\n", Power[Playernum - 1].popn);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Tech: %31d%8.3lf%8.3lf\n", total_invest, total_gain,
          total_max_gain);
  notify(Playernum, Governor, buf);
}
