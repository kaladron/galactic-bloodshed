// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 *  getplace -- returns directory level from string and game object
 *  Dispplace -- returns string from directory level
 *  testship(ship) -- tests various things for the ship.
 */

#include "gb/getplace.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static char Disps[PLACENAMESIZE];

char *Dispshiploc_brief(Ship *ship) {
  int i;

  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      sprintf(Disps, "/%-4.4s", Stars[ship->storbits]->name);
      return (Disps);
    case ScopeLevel::LEVEL_PLAN:
      sprintf(Disps, "/%s", Stars[ship->storbits]->name);
      for (i = 2; (Disps[i] && (i < 5)); i++)
        ;
      sprintf(Disps + i, "/%-4.4s",
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      return (Disps);
    case ScopeLevel::LEVEL_SHIP:
      sprintf(Disps, "#%lu", ship->destshipno);
      return (Disps);
    case ScopeLevel::LEVEL_UNIV:
      sprintf(Disps, "/");
      return (Disps);
  }
}

char *Dispshiploc(Ship *ship) {
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_STAR:
      sprintf(Disps, "/%s", Stars[ship->storbits]->name);
      return (Disps);
    case ScopeLevel::LEVEL_PLAN:
      sprintf(Disps, "/%s/%s", Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      return (Disps);
    case ScopeLevel::LEVEL_SHIP:
      sprintf(Disps, "#%lu", ship->destshipno);
      return (Disps);
    case ScopeLevel::LEVEL_UNIV:
      sprintf(Disps, "/");
      return (Disps);
  }
}
