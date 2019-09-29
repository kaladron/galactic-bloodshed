// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* moveplanet.c -- move the planet in orbit around its star. */

#include "gb/moveplanet.h"

import std;

#include "gb/doturn.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void moveplanet(int starnum, Planet *planet, int planetnum) {
  double dist;
  double xadd;
  double yadd;
  double phase;
  double period;
  int sh;
  Ship *ship;

  if (planet->popn || planet->ships) Stinfo[starnum][planetnum].inhab = 1;

  StarsInhab[starnum] =
      !!(Stars[starnum]->inhabited[0] + Stars[starnum]->inhabited[1]);
  StarsExpl[starnum] =
      !!(Stars[starnum]->explored[0] + Stars[starnum]->explored[1]);

  Stars[starnum]->inhabited[0] = Stars[starnum]->inhabited[1] = 0;
  if (!StarsExpl[starnum]) return; /* no one's explored the star yet */

  dist = hypot((double)(planet->ypos), (double)(planet->xpos));

  phase = atan2((double)(planet->ypos), (double)(planet->xpos));
  period =
      dist * sqrt((double)(dist / (SYSTEMGRAVCONST * Stars[starnum]->gravity)));
  /* keppler's law */

  xadd = dist * cos((double)(-1. / period + phase)) - planet->xpos;
  yadd = dist * sin((double)(-1. / period + phase)) - planet->ypos;
  /* one update time unit - planets orbit counter-clockwise */

  /* adjust ships in orbit around the planet */
  sh = planet->ships;
  while (sh) {
    ship = ships[sh];
    ship->xpos += xadd;
    ship->ypos += yadd;
    sh = ship->nextship;
  }

  planet->xpos += xadd;
  planet->ypos += yadd;
}
