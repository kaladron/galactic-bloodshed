// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file autoreport.c
/// \brief Tell server to generate a report for each planet.

#include "gb/autoreport.h"

#include <boost/format.hpp>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void autoreport(const command_t &argv, GameObj &g) {
  if (g.governor && Stars[g.snum]->governor[g.player - 1] != g.governor) {
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
      auto place = getplace(g, argv[1], 0);
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
  putplanet(p, Stars[snum], pnum);

  g.out << boost::format("Autoreport on %s has been %s.\n") %
               Stars[snum]->pnames[pnum] %
               (p.info[g.player - 1].autorep ? "set" : "unset");
}
