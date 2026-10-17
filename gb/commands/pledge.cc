// SPDX-License-Identifier: Apache-2.0

/// \file pledge.cc
/// \brief Pledge to join an alliance block.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

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

  GB::presentation::PledgeResult result{
      .pledger_id = Playernum,
      .pledger_name = g.race->name,
      .block_id = n,
      .bandwagon = (int_rand(1, 20) == 1),
  };

  g.entity_manager.mutate_block(n, [&](Block& b) {
    b.pledge(Playernum);
    result.block_name = b.name();
  });

  warn_race(g.session_registry, g.entity_manager, n,
            GB::presentation::render_pledge_block_notification(result));
  warn_race(g.session_registry, g.entity_manager, Playernum,
            GB::presentation::render_pledge_self_notification(result));
  post(g.entity_manager,
       GB::presentation::render_pledge_declaration_news(result),
       NewsType::DECLARATION);
  compute_power_blocks(g.entity_manager);

  g.present(result);
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