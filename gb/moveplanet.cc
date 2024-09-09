// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* moveplanet.c -- move the planet in orbit around its star. */

import gblib;
import std;

#include "gb/moveplanet.h"

void moveplanet(const int starnum, Planet &planet, const int planetnum) {
  if (planet.popn || planet.ships) Stinfo[starnum][planetnum].inhab = 1;

  StarsInhab[starnum] = !!(stars[starnum].inhabited);
  StarsExpl[starnum] = !!(stars[starnum].explored);

  stars[starnum].inhabited = 0;
  if (!StarsExpl[starnum]) return; /* no one's explored the star yet */

  double dist = std::hypot((double)(planet.ypos), (double)(planet.xpos));

  double phase = std::atan2((double)(planet.ypos), (double)(planet.xpos));
  double period =
      dist *
      std::sqrt((double)(dist / (SYSTEMGRAVCONST * stars[starnum].gravity)));
  /* keppler's law */

  double xadd = dist * std::cos((double)(-1. / period + phase)) - planet.xpos;
  double yadd = dist * std::sin((double)(-1. / period + phase)) - planet.ypos;
  /* one update time unit - planets orbit counter-clockwise */

  /* adjust ships in orbit around the planet */
  auto sh = planet.ships;
  while (sh) {
    auto ship = ships[sh];
    ship->xpos += xadd;
    ship->ypos += yadd;
    sh = ship->nextship;
  }

  planet.xpos += xadd;
  planet.ypos += yadd;
}
