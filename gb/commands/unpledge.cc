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
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();

  if (Governor) {
    g.out << "Only leaders may pledge.\n";
    return;
  }
  auto n = get_player(g.entity_manager, argv[1]);
  if (n == 0) {
    g.out << "No such player.\n";
    return;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }

  const auto* race = g.entity_manager.peek_race(Playernum);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  auto block_handle = g.entity_manager.get_block(n);
  if (!block_handle.get()) {
    g.out << "Block not found.\n";
    return;
  }
  auto& block = *block_handle;

  clrbit(block.pledge, Playernum);
  std::string quit_notification = std::format(
      "{} [{}] has quit {} [{}].\n", race->name, Playernum, block.name, n);
  warn_race(g.entity_manager, n, quit_notification);
  std::string player_notification =
      std::format("You have quit {}\n", block.name);
  warn_race(g.entity_manager, Playernum, player_notification);

  switch (int_rand(1, 20)) {
    case 1: {
      std::string taunt_postmsg =
          std::format("{} [{}] calls {} [{}] a bunch of geeks and QUITS!\n",
                      g.race->name, Playernum, block.name, n);
      post(g.entity_manager, taunt_postmsg, NewsType::DECLARATION);
      break;
    }
    default: {
      std::string quit_postmsg =
          std::format("{} [{}] has QUIT {} [{}]!\n", g.race->name, Playernum,
                      block.name, n);
      post(g.entity_manager, quit_postmsg, NewsType::DECLARATION);
      break;
    }
  }

  compute_power_blocks(g.entity_manager);
}
}  // namespace GB::commands