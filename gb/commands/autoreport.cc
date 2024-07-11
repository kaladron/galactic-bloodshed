// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file autoreport.c
/// \brief Tell server to generate a report for each planet.

import gblib;
import std.compat;

#include "gb/commands/autoreport.h"

#include "gb/GB_server.h"
#include "gb/files_shl.h"
#include "gb/place.h"
#include "gb/vars.h"

void autoreport(const command_t &argv, GameObj &g) {
  if (g.governor && stars[g.snum].governor[g.player - 1] != g.governor) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }

  starnum_t snum;
  planetnum_t pnum;

  switch (argv.size()) {
    case 1:
      if (g.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return;
      }
      snum = g.snum;
      pnum = g.pnum;
      break;
    case 2: {
      Place place{g, argv[1]};
      if (place.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Scope must be a planet.\n";
        return;
      }
      snum = place.snum;
      pnum = place.pnum;
    } break;
    default:
      g.out << "Invalid number of arguments.\n";
      return;
  }

  auto p = getplanet(snum, pnum);
  if (p.info[g.player - 1].autorep) {
    p.info[g.player - 1].autorep = 0;
  } else {
    p.info[g.player - 1].autorep = TELEG_MAX_AUTO;
  }
  putplanet(p, stars[snum], pnum);

  g.out << std::format("Autoreport on %{0} has been %{1}.\n",
                       stars[snum].pnames[pnum],
                       (p.info[g.player - 1].autorep ? "set" : "unset"));
}
