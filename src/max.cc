// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * maxsupport() -- return how many people one sector can support
 * compatibility() -- return how much race is compatible with planet
 * gravity() -- return gravity for planet
 * prin_ship_orbits() -- prints place ship orbits
 */

#include "max.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files_shl.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

static char Dispshiporbits_buf[PLACENAMESIZE + 13];

int maxsupport(racetype *r, sectortype *s, double c, int toxic) {
  int val;
  double a, b;

  if (!r->likes[s->condition])
    return 0.0;
  a = ((double)s->eff + 1.0) * (double)s->fert;
  b = (.01 * c);

  val = (int)(a * b * .01 * (100.0 - (double)toxic));

  return val;
}

double compatibility(planettype *planet, racetype *race) {
  int i, add;
  double sum, atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  add = 0.1 * ((double)planet->conditions[TEMP] - race->conditions[TEMP]);
  sum = 1.0 - (double)abs(add) / 100.0;

  /* step through and report compatibility of each planetary gas */
  for (i = TEMP + 1; i <= OTHER; i++) {
    add = (double)planet->conditions[i] - race->conditions[i];
    atmosphere *= 1.0 - (double)abs(add) / 100.0;
  }
  sum *= atmosphere;
  sum *= 100.0 - planet->conditions[TOXIC];

  if (sum < 0.0)
    return 0.0;
  return (sum);
}

double gravity(planettype *p) {
  return (double)(p->Maxx) * (double)(p->Maxy) * GRAV_FACTOR;
}

char *prin_ship_orbits(shiptype *s) {
  shiptype *mothership;
  char *motherorbits;

  switch (s->whatorbits) {
  case LEVEL_UNIV:
    sprintf(Dispshiporbits_buf, "/(%.0f,%.0f)", s->xpos, s->ypos);
    break;
  case LEVEL_STAR:
    sprintf(Dispshiporbits_buf, "/%s", Stars[s->storbits]->name);
    break;
  case LEVEL_PLAN:
    sprintf(Dispshiporbits_buf, "/%s/%s", Stars[s->storbits]->name,
            Stars[s->storbits]->pnames[s->pnumorbits]);
    break;
  case LEVEL_SHIP:
    if (getship(&mothership, s->destshipno)) {
      motherorbits = prin_ship_orbits(mothership);
      strcpy(Dispshiporbits_buf, motherorbits);
      free(mothership);
    } else
      strcpy(Dispshiporbits_buf, "/");
    break;
  }
  return Dispshiporbits_buf;
}
