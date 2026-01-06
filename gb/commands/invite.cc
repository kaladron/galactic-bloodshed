// SPDX-License-Identifier: Apache-2.0

// declare.c -- declare alliance, neutrality, war, the basic thing.

module;

import gblib;
import std.compat;
import notification;
import session;

module commands;

namespace GB::commands {
/* invite people to join your alliance block */
void invite(const command_t& argv, GameObj& g) {
  bool mode = argv[0] == "invite" ? true : false;

  player_t n;

  if (g.governor()) {
    g.out << "Only leaders may invite.\n";
    return;
  }
  if (!(n = get_player(g.entity_manager, argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (n == g.player()) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }

  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }
  const auto* alien = g.entity_manager.peek_race(n);
  if (!alien) {
    g.out << "Target race not found.\n";
    return;
  }

  auto block_handle = g.entity_manager.get_block(g.player());
  if (!block_handle.get()) {
    g.out << "Block not found.\n";
    return;
  }
  auto& block = *block_handle;

  std::string buf;
  if (mode) {
    setbit(block.invite, n);
    buf = std::format("{} [{}] has invited you to join {}\n", race->name,
                      g.player(), block.name);
    warn_race(g.session_registry, g.entity_manager, n, buf);
    buf = std::format("{} [{}] has been invited to join {} [{}]\n", alien->name,
                      n, block.name, g.player());
    warn_race(g.session_registry, g.entity_manager, g.player(), buf);
  } else {
    clrbit(block.invite, n);
    buf = std::format("You have been blackballed from {} [{}]\n", block.name,
                      g.player());
    warn_race(g.session_registry, g.entity_manager, n, buf);
    buf = std::format("{} [{}] has been blackballed from {} [{}]\n",
                      alien->name, n, block.name, g.player());
    warn_race(g.session_registry, g.entity_manager, g.player(), buf);
  }
  post(g.entity_manager, buf, NewsType::DECLARATION);
}
}  // namespace GB::commands
