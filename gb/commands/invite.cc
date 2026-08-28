// SPDX-License-Identifier: Apache-2.0

/// \file invite.cc
/// \brief Invite players to join an alliance block.

module;

import std;
import gb.entities;
import gb.services;
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

  const auto* alien = g.entity_manager.peek_race(n);
  if (!alien) {
    g.out << "Target race not found.\n";
    return false;
  }

  try {
    g.entity_manager.mutate_block(g.player().value, [&](auto& b) {
      std::string buf;
      if (mode) {
        setbit(b.invite, n);
        buf = std::format("{} [{}] has invited you to join {}\n", g.race->name,
                          g.player(), b.name);
        warn_race(g.session_registry, g.entity_manager, n, buf);
        buf = std::format("{} [{}] has been invited to join {} [{}]\n",
                          alien->name, n, b.name, g.player());
        warn_race(g.session_registry, g.entity_manager, g.player(), buf);
      } else {
        clrbit(b.invite, n);
        buf = std::format("You have been blackballed from {} [{}]\n", b.name,
                          g.player());
        warn_race(g.session_registry, g.entity_manager, n, buf);
        buf = std::format("{} [{}] has been blackballed from {} [{}]\n",
                          alien->name, n, b.name, g.player());
        warn_race(g.session_registry, g.entity_manager, g.player(), buf);
      }
      post(g.entity_manager, buf, NewsType::DECLARATION);
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Block not found.\n";
    return false;
  }
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
