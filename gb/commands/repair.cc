// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std;

#include <ctype.h>
#include <strings.h>

module commands;

namespace GB::commands {
void repair(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  int hix;
  int lowy;
  int hiy;
  int x2;

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level(), g.snum(), g.pnum());
  } else {
    /* repairing a sector */
    if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
      if (g.level() != ScopeLevel::LEVEL_PLAN) {
        g.out << "There are no sectors here.\n";
        return;
      }
      where =
          std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum(), g.pnum());

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return;
    }
  }

  if (where->level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Scope must be a planet.\n";
  }

  auto planet_handle = g.entity_manager.get_planet(where->snum, where->pnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;
  if (!p.info(Playernum).numsectsowned) {
    g.out << "You don't own any sectors on this planet.\n";
    return;
  }

  auto smap_handle = g.entity_manager.get_sectormap(where->snum, where->pnum);
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return;
  }
  auto& smap = *smap_handle;
  if (isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) {
    // translate from lowx:hix,lowy:hiy
    auto coords = get4args(argv[1]);
    if (!coords) {
      g.out << "Invalid coordinate format. Use: x,y or xl:xh,yl:yh\n";
      return;
    }
    auto [x_low, x_high, y_low, y_high] = *coords;
    x2 = std::max(0, x_low);
    hix = std::min(x_high, p.Maxx() - 1);
    lowy = std::max(0, y_low);
    hiy = std::min(y_high, p.Maxy() - 1);
  } else {
    /* repair entire planet */
    x2 = 0;
    hix = p.Maxx() - 1;
    lowy = 0;
    hiy = p.Maxy() - 1;
  }

  int sectors = 0;
  int cost = 0;
  for (; lowy <= hiy; lowy++) {
    for (int lowx = x2; lowx <= hix; lowx++) {
      if (p.info(Playernum).resource >= SECTOR_REPAIR_COST) {
        auto& s = smap.get(lowx, lowy);
        if (s.is_wasted() && (s.get_owner() == Playernum || !s.is_owned())) {
          s.set_condition(s.get_type());
          s.set_fert(std::min(100U, s.get_fert() + 20));
          p.info(Playernum).resource -= SECTOR_REPAIR_COST;
          cost += SECTOR_REPAIR_COST;
          sectors += 1;
        }
      }
    }
  }

  g.out << std::format("{0} sectors repaired at a cost of {1} resources.\n",
                       sectors, cost);
}
}  // namespace GB::commands
