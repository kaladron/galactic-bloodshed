// Copyright 2020 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void center(const command_t& argv, GameObj& g) {
  if (argv.size() != 2) {
    g.out << "center: which star?\n";
  }
  Place where{g, argv[1], true};

  if (where.err) {
    g.out << "center: bad scope.\n";
    return;
  }
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    g.out << "CHEATER!!!\n";
    return;
  }
  const auto* star = g.entity_manager.peek_star(where.snum);
  if (!star) {
    g.out << "center: star data not found.\n";
    return;
  }
  g.lastx[1] = star->xpos();
  g.lasty[1] = star->ypos();
}
}  // namespace GB::commands
