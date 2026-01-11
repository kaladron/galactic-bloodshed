// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file highlight.cc
/// \brief Toggle highlight option on a player.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void highlight(const command_t& argv, GameObj& g) {
  player_t n{0};

  n = get_player(g.entity_manager, argv[1]);
  if (n.value == 0) {
    g.out << "No such player.\n";
    return;
  }

  // Get race for modification (RAII auto-saves on scope exit)
  auto race_handle = g.entity_manager.get_race(g.player());
  auto& race = *race_handle;
  race.governor[g.governor().value].toggle.highlight = n;
}
}  // namespace GB::commands
