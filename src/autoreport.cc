// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * autoreport.c -- tell server to generate a report for each planet
 */

#include "autoreport.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "getplace.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

void autoreport(const command_t &argv, const player_t Playernum,
                const governor_t Governor) {
  placetype place;
  starnum_t snum;
  planetnum_t pnum;

  snum = Dir[Playernum - 1][Governor].snum;
  pnum = Dir[Playernum - 1][Governor].pnum;

  if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor, "You are not authorized to do this here.\n");
    return;
  }

  if (argn == 1) { /* no args */
    if (Dir[Playernum - 1][Governor].level == LEVEL_PLAN) {
      auto p = getplanet(snum, pnum);
      if (p.info[Playernum - 1].autorep)
        p.info[Playernum - 1].autorep = 0;
      else
        p.info[Playernum - 1].autorep = TELEG_MAX_AUTO;
      putplanet(p, Stars[snum], pnum);

      sprintf(buf, "Autoreport on %s has been %s.\n", Stars[snum]->pnames[pnum],
              p.info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "Scope must be a planet.\n");
      notify(Playernum, Governor, buf);
    }
  } else if (argv.size() > 1) { /* argn==2, place specified */
    place = Getplace(Playernum, Governor, argv[1], 0);
    if (place.level == LEVEL_PLAN) {
      auto p = getplanet(snum, pnum);
      sprintf(buf, "Autoreport on %s has been %s.\n", Stars[snum]->pnames[pnum],
              p.info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
      p.info[Playernum - 1].autorep = !p.info[Playernum - 1].autorep;
      putplanet(p, Stars[snum], pnum);
    } else {
      sprintf(buf, "Scope must be a planet.\n");
      notify(Playernum, Governor, buf);
    }
  }
}
