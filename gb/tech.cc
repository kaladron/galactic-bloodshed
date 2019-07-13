// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* tech.c -- increase investment in technological development. */

#include "gb/tech.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/mobiliz.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void technology(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 1;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet (%d).\n", g.level);
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[g.snum])) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (argv.size() < 2) {
    sprintf(buf,
            "Current investment : %d    Technology production/update: %.3f\n",
            p.info[Playernum - 1].tech_invest,
            tech_prod((int)(p.info[Playernum - 1].tech_invest),
                      (int)(p.info[Playernum - 1].popn)));
    notify(Playernum, Governor, buf);
    return;
  }
  short invest = std::stoi(argv[1]);

  if (invest < 0) {
    g.out << "Illegal value.\n";
    return;
  }

  p.info[Playernum - 1].tech_invest = invest;

  putplanet(p, Stars[g.snum], g.pnum);

  deductAPs(Playernum, Governor, APcount, g.snum, 0);

  sprintf(buf, "   New (ideal) tech production: %.3f (this planet)\n",
          tech_prod((int)(p.info[Playernum - 1].tech_invest),
                    (int)(p.info[Playernum - 1].popn)));
  notify(Playernum, Governor, buf);
}

double tech_prod(int investment, int popn) {
  double scale = (double)popn / 10000.;
  return (TECH_INVEST * log10((double)investment * scale + 1.0));
}