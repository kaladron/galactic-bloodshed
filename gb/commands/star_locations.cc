// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/star_locations.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/place.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tech.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void star_locations(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int i;
  double dist;
  double x;
  double y;
  int max;

  x = g.lastx[1];
  y = g.lasty[1];

  if (argv.size() > 1)
    max = std::stoi(argv[1]);
  else
    max = 999999;

  for (i = 0; i < Sdata.numstars; i++) {
    dist = sqrt(Distsq(Stars[i]->xpos, Stars[i]->ypos, x, y));
    if ((int)dist <= max) {
      sprintf(buf, "(%2d) %20.20s (%8.0f,%8.0f) %7.0f\n", i, Stars[i]->name,
              Stars[i]->xpos, Stars[i]->ypos, dist);
      notify(Playernum, Governor, buf);
    }
  }
}
