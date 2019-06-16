// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * maxsupport() -- return how many people one sector can support
 * compatibility() -- return how much race is compatible with planet
 * gravity() -- return gravity for planet
 * prin_ship_orbits() -- prints place ship orbits
 */

#include "gb/max.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static char Dispshiporbits_buf[PLACENAMESIZE + 13];

int maxsupport(const Race *r, const Sector &s, const double c,
               const int toxic) {
  if (!r->likes[s.condition]) return 0.0;
  double a = ((double)s.eff + 1.0) * (double)s.fert;
  double b = (.01 * c);

  int val = (int)(a * b * .01 * (100.0 - (double)toxic));

  return val;
}

double compatibility(const Planet &planet, const Race *race) {
  int i;
  int add;
  double sum;
  double atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  add = 0.1 * ((double)planet.conditions[TEMP] - race->conditions[TEMP]);
  sum = 1.0 - (double)abs(add) / 100.0;

  /* step through and report compatibility of each planetary gas */
  for (i = TEMP + 1; i <= OTHER; i++) {
    add = (double)planet.conditions[i] - race->conditions[i];
    atmosphere *= 1.0 - (double)abs(add) / 100.0;
  }
  sum *= atmosphere;
  sum *= 100.0 - planet.conditions[TOXIC];

  if (sum < 0.0) return 0.0;
  return (sum);
}

double gravity(const Planet &p) {
  return (double)(p.Maxx) * (double)(p.Maxy) * GRAV_FACTOR;
}

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
