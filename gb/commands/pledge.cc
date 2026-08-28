// SPDX-License-Identifier: Apache-2.0

/// \file pledge.cc
/// \brief Pledge to join an alliance block.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import session;

module commands;

namespace GB::commands {
/* declare that you wish to be included in the alliance block */
bool pledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  player_t n = get_player(g.entity_manager, argv[1]);
  if (n == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return false;
  }

  try {
    g.entity_manager.mutate_block(n.value, [&](auto& b) {
      setbit(b.pledge, Playernum);
      warn_race(g.session_registry, g.entity_manager, n,
                std::format("{} [{}] has pledged {}.\n", g.race->name,
                            Playernum, b.name));
      warn_race(g.session_registry, g.entity_manager, Playernum,
                std::format("You have pledged allegiance to {}.\n", b.name));

      std::string msg;
      switch (int_rand(1, 20)) {
        case 1:
          msg = std::format("{} [{}] joins the band wagon and pledges "
                            "allegiance to {} [{}]!\n",
                            g.race->name, Playernum, b.name, n);
          break;
        default:
          msg = std::format("{} [{}] pledges allegiance to {} [{}].\n",
                            g.race->name, Playernum, b.name, n);
          break;
      }

      post(g.entity_manager, msg, NewsType::DECLARATION);
    });
    compute_power_blocks(g.entity_manager);
  } catch (const EntityNotFoundError&) {
    g.out << "Block not found.\n";
    return false;
  }
  return true;
}

const CommandDescriptor pledge_cmd{
    .name = "pledge",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "pledge <race>",
    .description = "Pledge allegiance to join an alliance block",
    .handler = &pledge,
};

}  // namespace GB::commands