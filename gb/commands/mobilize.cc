// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file mobilize.c
/// \brief Persuade people to build military stuff.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"

module commands;

void mobilize(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (argv.size() < 2) {
    sprintf(buf, "Current mobilization: %d    Quota: %d\n",
            p.info[Playernum - 1].comread, p.info[Playernum - 1].mob_set);
    notify(Playernum, Governor, buf);
    return;
  }
  int sum_mob = std::stoi(argv[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  p.info[Playernum - 1].mob_set = sum_mob;
  putplanet(p, stars[g.snum], g.pnum);
  deductAPs(g, APcount, g.snum);
}
