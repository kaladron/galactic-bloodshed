// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * maxsupport() -- return how many people one sector can support
 * compatibility() -- return how much race is compatible with planet
 * prin_ship_orbits() -- prints place ship orbits
 */

import gblib;
import std;

#include "gb/max.h"

#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static char Dispshiporbits_buf[PLACENAMESIZE + 13];

char *prin_ship_orbits(Ship *s) {
  char *motherorbits;

  switch (s->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      sprintf(Dispshiporbits_buf, "/(%.0f,%.0f)", s->xpos, s->ypos);
      break;
    case ScopeLevel::LEVEL_STAR:
      sprintf(Dispshiporbits_buf, "/%s", Stars[s->storbits]->name);
      break;
    case ScopeLevel::LEVEL_PLAN:
      sprintf(Dispshiporbits_buf, "/%s/%s", Stars[s->storbits]->name,
              Stars[s->storbits]->pnames[s->pnumorbits]);
      break;
    case ScopeLevel::LEVEL_SHIP:
      if (auto mothership = getship(s->destshipno); mothership) {
        motherorbits = prin_ship_orbits(&*mothership);
        strcpy(Dispshiporbits_buf, motherorbits);
      } else
        strcpy(Dispshiporbits_buf, "/");
      break;
  }
  return Dispshiporbits_buf;
}
