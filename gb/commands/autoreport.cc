// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file autoreport.c
/// \brief Tell server to generate a report for each planet.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void autoreport(const command_t& argv, GameObj& g) {
  const auto* star = g.entity_manager.peek_star(g.snum);
  if (!star) {
    g.out << "Star not found.\n";
    return;
  }

  if (g.governor && star->governor[g.player - 1] != g.governor) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }

  starnum_t snum = 0;
  planetnum_t pnum = 0;

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

  // Get planet for modification (RAII auto-saves on scope exit)
  auto planet_handle = g.entity_manager.get_planet(snum, pnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }

  auto& p = *planet_handle;
  if (p.info[g.player - 1].autorep) {
    p.info[g.player - 1].autorep = 0;
  } else {
    p.info[g.player - 1].autorep = TELEG_MAX_AUTO;
  }

  // Get star name for output message
  const auto* target_star = g.entity_manager.peek_star(snum);
  g.out << std::format("Autoreport on {0} has been {1}.\n",
                       target_star ? target_star->pnames[pnum] : "Unknown",
                       (p.info[g.player - 1].autorep ? "set" : "unset"));
}
}  // namespace GB::commands
