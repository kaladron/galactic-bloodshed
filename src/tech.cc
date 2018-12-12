// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* tech.c -- increase investment in technological development. */

#include "tech.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "mobiliz.h"
#include "shlmisc.h"
#include "tweakables.h"
#include "vars.h"

void technology(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 1;
  short invest;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet (%d).\n", g.level);
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  auto p = getplanet(Dir[Playernum - 1][Governor].snum,
                     Dir[Playernum - 1][Governor].pnum);

  if (argn < 2) {
    sprintf(buf,
            "Current investment : %d    Technology production/update: %.3f\n",
            p.info[Playernum - 1].tech_invest,
            tech_prod((int)(p.info[Playernum - 1].tech_invest),
                      (int)(p.info[Playernum - 1].popn)));
    notify(Playernum, Governor, buf);
    return;
  }
  invest = atoi(args[1]);

  if (invest < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  p.info[Playernum - 1].tech_invest = invest;

  putplanet(p, Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].pnum);

  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  sprintf(buf, "   New (ideal) tech production: %.3f (this planet)\n",
          tech_prod((int)(p.info[Playernum - 1].tech_invest),
                    (int)(p.info[Playernum - 1].popn)));
  notify(Playernum, Governor, buf);
}

double tech_prod(int investment, int popn) {
  double scale;

  scale = (double)popn / 10000.;
  return (TECH_INVEST * log10((double)investment * scale + 1.0));
}
