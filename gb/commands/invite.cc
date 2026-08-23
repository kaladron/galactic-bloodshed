// SPDX-License-Identifier: Apache-2.0

/// \file invite.cc
/// \brief Invite players to join an alliance block.

module;

import std;
import gblib;
import notification;
import session;

module commands;

namespace GB::commands {
/* invite people to join your alliance block */
bool invite(const command_t& argv, GameObj& g) {
  bool mode = argv[0] == "invite";

  player_t n = get_player(g.entity_manager, argv[1]);
  if (n.value == 0) {
    g.out << "No such player.\n";
    return false;
  }
  if (n == g.player()) {
    g.out << "Not needed, you are the leader.\n";
    return false;
  }

  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Race not found.\n";
    return false;
  }
  const auto* alien = g.entity_manager.peek_race(n);
  if (!alien) {
    g.out << "Target race not found.\n";
    return false;
  }

  auto block_handle = g.entity_manager.get_block(g.player().value);
  if (!block_handle.get()) {
    g.out << "Block not found.\n";
    return false;
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
  return true;
}

const CommandDescriptor invite_cmd{
    .name = "invite",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "invite <player>",
    .description = "Invite a player to join your alliance block",
    .handler = &invite,
};

const CommandDescriptor uninvite_cmd{
    .name = "uninvite",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "uninvite <player>",
    .description = "Blackball/uninvite a player from your alliance block",
    .handler = &invite,
};

}  // namespace GB::commands
