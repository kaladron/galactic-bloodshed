// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * autoreport.c -- tell server to generate a report for each planet
 */

#include "autoreport.h"

#include <cstdio>
#include <cstdlib>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "getplace.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

void autoreport(const command_t &argv, GameObj &g) {
  const int Playernum = g.player;
  const int Governor = g.governor;

  if (Governor && Stars[g.snum]->governor[Playernum - 1] != Governor) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }

  if (argv.size() == 1) { /* no args */
    if (g.level == ScopeLevel::LEVEL_PLAN) {
      auto p = getplanet(g.snum, g.pnum);
      if (p.info[Playernum - 1].autorep)
        p.info[Playernum - 1].autorep = 0;
      else
        p.info[Playernum - 1].autorep = TELEG_MAX_AUTO;
      putplanet(p, Stars[g.snum], g.pnum);

      sprintf(buf, "Autoreport on %s has been %s.\n",
              Stars[g.snum]->pnames[g.pnum],
              p.info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
    } else {
      g.out << "Scope must be a planet.\n";
    }
  } else if (argv.size() > 1) { /* argv.size()==2, place specified */
    auto place = Getplace(g, argv[1], 0);
    if (place.level == ScopeLevel::LEVEL_PLAN) {
      auto p = getplanet(g.snum, g.pnum);
      sprintf(buf, "Autoreport on %s has been %s.\n",
              Stars[g.snum]->pnames[g.pnum],
              p.info[Playernum - 1].autorep ? "set" : "unset");
      notify(Playernum, Governor, buf);
      p.info[Playernum - 1].autorep = !p.info[Playernum - 1].autorep;
      putplanet(p, Stars[g.snum], g.pnum);
    } else {
      g.out << "Scope must be a planet.\n";
    }
  }
}
