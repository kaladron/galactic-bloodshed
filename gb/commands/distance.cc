// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/distance.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/place.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void distance(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  double x0;
  double y0;
  double x1;
  double y1;
  double dist;

  if (argv.size() < 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return;
  }

  Place from{g, argv[1], true};
  if (from.err) {
    sprintf(buf, "Bad scope '%s'.\n", argv[1].c_str());
    notify(Playernum, Governor, buf);
    return;
  }
  Place to{g, argv[2], true};
  if (to.err) {
    sprintf(buf, "Bad scope '%s'.\n", argv[2].c_str());
    notify(Playernum, Governor, buf);
  }

  x0 = 0.0;
  y0 = 0.0;
  x1 = 0.0;
  y1 = 0.0;
  /* get position in absolute units */
  if (from.level == ScopeLevel::LEVEL_SHIP) {
    auto ship = getship(from.shipno);
    if (ship->owner != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x0 = ship->xpos;
    y0 = ship->ypos;
  } else if (from.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(from.snum, from.pnum);
    x0 = p.xpos + stars[from.snum].xpos;
    y0 = p.ypos + stars[from.snum].ypos;
  } else if (from.level == ScopeLevel::LEVEL_STAR) {
    x0 = stars[from.snum].xpos;
    y0 = stars[from.snum].ypos;
  }

  if (to.level == ScopeLevel::LEVEL_SHIP) {
    auto ship = getship(to.shipno);
    if (ship->owner != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x1 = ship->xpos;
    y1 = ship->ypos;
  } else if (to.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(to.snum, to.pnum);
    x1 = p.xpos + stars[to.snum].xpos;
    y1 = p.ypos + stars[to.snum].ypos;
  } else if (to.level == ScopeLevel::LEVEL_STAR) {
    x1 = stars[to.snum].xpos;
    y1 = stars[to.snum].ypos;
  }
  /* compute the distance */
  dist = sqrt(Distsq(x0, y0, x1, y1));
  sprintf(buf, "Distance = %f\n", dist);
  notify(Playernum, Governor, buf);
}
