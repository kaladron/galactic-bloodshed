// Copyright 2020 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/center.h"

#include "gb/place.h"
#include "gb/vars.h"

void center(const command_t &argv, GameObj &g) {
  if (argv.size() != 2) {
    g.out << "center: which star?\n";
  }
  Place where{g, argv[1], true};

  if (where.err) {
    g.out << "center: bad scope.\n";
    return;
  }
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    g.out << "CHEATER!!!\n";
    return;
  }
  g.lastx[1] = stars[where.snum].xpos;
  g.lasty[1] = stars[where.snum].ypos;
}
