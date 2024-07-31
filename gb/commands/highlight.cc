// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file highlight.cc
/// \brief Toggle highlight option on a player.

module;

import gblib;
import std.compat;

#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"

module commands;

void highlight(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  player_t n;

  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  auto &race = races[Playernum - 1];
  race.governor[Governor].toggle.highlight = n;
  putrace(race);
}
