// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* technology.c -- increase investment in technological development. */

import gblib;
import std.compat;

#include "gb/commands/technology.h"

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
  ap_t APcount = 1;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet (%d).\n", g.level);
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (argv.size() < 2) {
    sprintf(buf,
            "Current investment : %ld    Technology production/update: %.3f\n",
            p.info[Playernum - 1].tech_invest,
            tech_prod(p.info[Playernum - 1].tech_invest,
                      p.info[Playernum - 1].popn));
    notify(Playernum, Governor, buf);
    return;
  }
  money_t invest = std::stoi(argv[1]);

  if (invest < 0) {
    g.out << "Illegal value.\n";
    return;
  }

  p.info[Playernum - 1].tech_invest = invest;

  putplanet(p, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);

  sprintf(
      buf, "   New (ideal) tech production: %.3f (this planet)\n",
      tech_prod(p.info[Playernum - 1].tech_invest, p.info[Playernum - 1].popn));
  notify(Playernum, Governor, buf);
}
