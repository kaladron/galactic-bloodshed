// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/repair.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/csp.h"
#include "gb/csp_types.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void repair(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int lowx;
  int hix;
  int lowy;
  int hiy;
  int x2;
  int sectors;
  int cost;

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level, g.snum, g.pnum);
  } else {
    /* repairing a sector */
    if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
      if (g.level != ScopeLevel::LEVEL_PLAN) {
        sprintf(buf, "There are no sectors here.\n");
        notify(Playernum, Governor, buf);
        return;
      }
      where = std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum, g.pnum);

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return;
    }
  }

  if (where->level == ScopeLevel::LEVEL_PLAN) {
    auto p = getplanet(where->snum, where->pnum);
    if (!p.info[Playernum - 1].numsectsowned) {
      notify(Playernum, Governor,
             "You don't own any sectors on this planet.\n");
      return;
    }
    auto smap = getsmap(p);
    if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
      get4args(argv[1].c_str(), &x2, &hix, &lowy, &hiy);
      /* ^^^ translate from lowx:hix,lowy:hiy */
      x2 = std::max(0, x2);
      hix = std::min(hix, p.Maxx - 1);
      lowy = std::max(0, lowy);
      hiy = std::min(hiy, p.Maxy - 1);
    } else {
      /* repair entire planet */
      x2 = 0;
      hix = p.Maxx - 1;
      lowy = 0;
      hiy = p.Maxy - 1;
    }
    sectors = 0;
    cost = 0;

    for (; lowy <= hiy; lowy++)
      for (lowx = x2; lowx <= hix; lowx++) {
        if (p.info[Playernum - 1].resource >= SECTOR_REPAIR_COST) {
          auto &s = smap.get(lowx, lowy);
          if (s.condition == SectorType::SEC_WASTED &&
              (s.owner == Playernum || !s.owner)) {
            s.condition = s.type;
            s.fert = std::min(100U, s.fert + 20);
            p.info[Playernum - 1].resource -= SECTOR_REPAIR_COST;
            cost += SECTOR_REPAIR_COST;
            sectors += 1;
            putsector(s, p, lowx, lowy);
          }
        }
      }
    putplanet(p, Stars[where->snum], where->pnum);

    sprintf(buf, "%d sectors repaired at a cost of %d resources.\n", sectors,
            cost);
    notify(Playernum, Governor, buf);
  } else {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
  }
}
