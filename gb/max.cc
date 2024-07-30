// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * maxsupport() -- return how many people one sector can support
 * compatibility() -- return how much race is compatible with planet
 * prin_ship_orbits() -- prints place ship orbits
 */

import gblib;
import std.compat;

#include "gb/max.h"

#include "gb/files_shl.h"
#include "gb/ships.h"

std::string prin_ship_orbits(const Ship &s) {
  switch (s.whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      return std::format("/({:0.0},{:1.0})", s.xpos, s.ypos);
    case ScopeLevel::LEVEL_STAR:
      return std::format("/{0}", stars[s.storbits].name);
    case ScopeLevel::LEVEL_PLAN:
      return std::format("/{0}/{1}", stars[s.storbits].name,
                         stars[s.storbits].pnames[s.pnumorbits]);
    case ScopeLevel::LEVEL_SHIP:
      if (auto mothership = getship(s.destshipno); mothership) {
        return prin_ship_orbits(*mothership);
      } else {
        return "/";
      }
  }
}
