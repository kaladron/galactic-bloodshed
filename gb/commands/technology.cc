// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* technology.c -- increase investment in technological development. */

import gblib;

#include "gb/commands/technology.h"

import std;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/shlmisc.h"
#include "gb/star.h"
#include "gb/tech.h"
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
  if (!control(*Stars[g.snum], Playernum, Governor)) {
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
