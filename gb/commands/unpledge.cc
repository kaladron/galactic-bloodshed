// Copyright 2020 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/config.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/vars.h"

module commands;

/* declare that you wish to be included in the alliance block */
void unpledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int n;

  if (Governor) {
    g.out << "Only leaders may pledge.\n";
    return;
  }
  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }
  auto& race = races[Playernum - 1];
  clrbit(Blocks[n - 1].pledge, Playernum);
  sprintf(buf, "%s [%d] has quit %s [%d].\n", race.name, Playernum,
          Blocks[n - 1].name, n);
  warn_race(n, buf);
  sprintf(buf, "You have quit %s\n", Blocks[n - 1].name);
  warn_race(Playernum, buf);

  switch (int_rand(1, 20)) {
    case 1:
      sprintf(buf, "%s [%d] calls %s [%d] a bunch of geeks and QUITS!\n",
              race.name, Playernum, Blocks[n - 1].name, n);
      break;
    default:
      sprintf(buf, "%s [%d] has QUIT %s [%d]!\n", race.name, Playernum,
              Blocks[n - 1].name, n);
      break;
  }

  post(buf, DECLARATION);

  compute_power_blocks();
  Putblock(Blocks);
}
