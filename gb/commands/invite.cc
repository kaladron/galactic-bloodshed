// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
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
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

module commands;

/* invite people to join your alliance block */
void invite(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  bool mode = argv[0] == "invite";

  player_t n;

  if (Governor) {
    g.out << "Only leaders may invite.\n";
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
  auto& alien = races[n - 1];
  if (mode) {
    setbit(Blocks[Playernum - 1].invite, n);
    sprintf(buf, "%s [%d] has invited you to join %s\n", race.name, Playernum,
            Blocks[Playernum - 1].name);
    warn_race(n, buf);
    sprintf(buf, "%s [%d] has been invited to join %s [%d]\n", alien.name, n,
            Blocks[Playernum - 1].name, Playernum);
    warn_race(Playernum, buf);
  } else {
    clrbit(Blocks[Playernum - 1].invite, n);
    sprintf(buf, "You have been blackballed from %s [%d]\n",
            Blocks[Playernum - 1].name, Playernum);
    warn_race(n, buf);
    sprintf(buf, "%s [%d] has been blackballed from %s [%d]\n", alien.name, n,
            Blocks[Playernum - 1].name, Playernum);
    warn_race(Playernum, buf);
  }
  post(buf, DECLARATION);

  Putblock(Blocks);
}
