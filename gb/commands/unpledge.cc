// SPDX-License-Identifier: Apache-2.0

/// \file unpledge.cc
/// \brief Withdraw pledge to an alliance block.

module;

import std;
import gb.entities;
import gb.services;
import notification;

module commands;

namespace GB::commands {
/* declare that you wish to withdraw from the alliance block */
bool unpledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  auto n = get_player(g.entity_manager, argv[1]);
  if (n == 0) {
    g.out << "No such player.\n";
    return false;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return false;
  }

  try {
    g.entity_manager.mutate_block(n.value, [&](auto& b) {
      clrbit(b.pledge, Playernum);
      std::string quit_notification = std::format(
          "{} [{}] has quit {} [{}].\n", g.race->name, Playernum, b.name, n);
      warn_race(g.session_registry, g.entity_manager, n, quit_notification);
      std::string player_notification =
          std::format("You have quit {}\n", b.name);
      warn_race(g.session_registry, g.entity_manager, Playernum,
                player_notification);

      switch (int_rand(1, 20)) {
        case 1: {
          std::string taunt_postmsg =
              std::format("{} [{}] calls {} [{}] a bunch of geeks and QUITS!\n",
                          g.race->name, Playernum, b.name, n);
          post(g.entity_manager, taunt_postmsg, NewsType::DECLARATION);
          break;
        }
        default: {
          std::string quit_postmsg =
              std::format("{} [{}] has QUIT {} [{}]!\n", g.race->name,
                          Playernum, b.name, n);
          post(g.entity_manager, quit_postmsg, NewsType::DECLARATION);
          break;
        }
      }
    });
    compute_power_blocks(g.entity_manager);
  } catch (const EntityNotFoundError&) {
    g.out << "Block not found.\n";
    return false;
  }
  return true;
}

const CommandDescriptor unpledge_cmd{
    .name = "unpledge",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "unpledge <race>",
    .description = "Withdraw pledge and leave an alliance block",
    .handler = &unpledge,
};

}  // namespace GB::commands