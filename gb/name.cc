// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* name.c -- rename something to something else */

import gblib;
import std.compat;

#include "gb/name.h"

int revolt(Planet &pl, const player_t victim, const player_t agent) {
  int revolted_sectors = 0;

  auto smap = getsmap(pl);
  for (auto &s : smap) {
    if (s.owner != victim || s.popn == 0) continue;

    // Revolt rate is a function of tax rate.
    if (!success(pl.info[victim - 1].tax)) continue;

    if (static_cast<unsigned long>(long_rand(1, s.popn)) <=
        10 * races[victim - 1].fighters * s.troops)
      continue;

    // Revolt successful.
    s.owner = agent;                   /* enemy gets it */
    s.popn = int_rand(1, (int)s.popn); /* some people killed */
    s.troops = 0;                      /* all troops destroyed */
    pl.info[victim - 1].numsectsowned -= 1;
    pl.info[agent - 1].numsectsowned += 1;
    pl.info[victim - 1].mob_points -= s.mobilization;
    pl.info[agent - 1].mob_points += s.mobilization;
    revolted_sectors++;
  }
  putsmap(smap, pl);

  return revolted_sectors;
}
