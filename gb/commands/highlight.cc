// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file highlight.cc
/// \brief Toggle highlight option on a player.

#include "gb/commands/highlight.h"

#include <cstdio>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/vars.h"

void highlight(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  player_t n;
  racetype *Race;

  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  Race = races[Playernum - 1];
  Race->governor[Governor].toggle.highlight = n;
  putrace(Race);
}
