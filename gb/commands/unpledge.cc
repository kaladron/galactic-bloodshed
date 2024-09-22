// SPDX-License-Identifier: Apache-2.0

// declare.c -- declare alliance, neutrality, war, the basic thing.

module;

import gblib;
import std;

#include "gb/GB_server.h"

module commands;

namespace GB::commands {
/* declare that you wish to be included in the alliance block */
void unpledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (Governor) {
    g.out << "Only leaders may pledge.\n";
    return;
  }
  auto n = get_player(argv[1]);
  if (n == 0) {
    g.out << "No such player.\n";
    return;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }
  auto& race = races[Playernum - 1];
  clrbit(Blocks[n - 1].pledge, Playernum);
  std::string quit_notification =
      std::format("{} [{}] has quit {} [{}].\n", race.name, Playernum,
                  Blocks[n - 1].name, n);
  warn_race(n, quit_notification);
  std::string player_notification =
      std::format("You have quit {}\n", Blocks[n - 1].name);
  warn_race(Playernum, player_notification);

  switch (int_rand(1, 20)) {
    case 1: {
      std::string taunt_postmsg =
          std::format("{} [{}] calls {} [{}] a bunch of geeks and QUITS!\n",
                      race.name, Playernum, Blocks[n - 1].name, n);
      post(taunt_postmsg, NewsType::DECLARATION);
      break;
    }
    default: {
      std::string quit_postmsg =
          std::format("{} [{}] has QUIT {} [{}]!\n", race.name, Playernum,
                      Blocks[n - 1].name, n);
      post(quit_postmsg, NewsType::DECLARATION);
      break;
    }
  }

  compute_power_blocks();
  Putblock(Blocks);
}
}  // namespace GB::commands