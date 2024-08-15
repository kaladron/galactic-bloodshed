// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/tweakables.h"

module commands;

namespace GB::commands {
void repair(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  int hix;
  int lowy;
  int hiy;
  int x2;

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level, g.snum, g.pnum);
  } else {
    /* repairing a sector */
    if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
      if (g.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "There are no sectors here.\n";
        return;
      }
      where = std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum, g.pnum);

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return;
    }
  }

  if (where->level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Scope must be a planet.\n";
  }

  auto p = getplanet(where->snum, where->pnum);
  if (!p.info[Playernum - 1].numsectsowned) {
    g.out << "You don't own any sectors on this planet.\n";
    return;
  }

  auto smap = getsmap(p);
  if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
    // translate from lowx:hix,lowy:hiy
    get4args(argv[1].c_str(), &x2, &hix, &lowy, &hiy);
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

  int sectors = 0;
  int cost = 0;
  for (; lowy <= hiy; lowy++) {
    for (int lowx = x2; lowx <= hix; lowx++) {
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
  }
  putplanet(p, stars[where->snum], where->pnum);

  g.out << std::format("{0} sectors repaired at a cost of {1} resources.\n",
                       sectors, cost);
}
}  // namespace GB::commands
