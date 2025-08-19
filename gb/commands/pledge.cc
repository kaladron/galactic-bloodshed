// Copyright 2020 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"

module commands;

namespace GB::commands {
/* declare that you wish to be included in the alliance block */
void pledge(const command_t& argv, GameObj& g) {
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
  setbit(Blocks[n - 1].pledge, Playernum);
  warn_race(n, std::format("{} [{}] has pledged {}.\n", race.name, Playernum,
                           Blocks[n - 1].name));
  warn_race(Playernum, std::format("You have pledged allegiance to {}.\n",
                                   Blocks[n - 1].name));

  std::string msg;
  switch (int_rand(1, 20)) {
    case 1:
      msg = std::format(
          "{} [{}] joins the band wagon and pledges allegiance to {} [{}]!\n",
          race.name, Playernum, Blocks[n - 1].name, n);
      break;
    default:
      msg = std::format("{} [{}] pledges allegiance to {} [{}].\n", race.name,
                        Playernum, Blocks[n - 1].name, n);
      break;
  }

  post(msg, NewsType::DECLARATION);

  compute_power_blocks();
  Putblock(Blocks);
}
}  // namespace GB::commands