// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"

module commands;

namespace GB::commands {
void star_locations(const command_t &argv, GameObj &g) {
  int max;
  if (argv.size() > 1)
    max = std::stoi(argv[1]);
  else
    max = 999999;

  for (auto i = 0; i < Sdata.numstars; i++) {
    auto dist =
        sqrt(Distsq(stars[i].xpos, stars[i].ypos, g.lastx[1], g.lasty[1]));
    if (std::floor(dist) <= max) {
      sprintf(buf, "(%2d) %20.20s (%8.0f,%8.0f) %7.0f\n", i, stars[i].name,
              stars[i].xpos, stars[i].ypos, dist);
      notify(g.player, g.governor, buf);
    }
  }
}
}  // namespace GB::commands
