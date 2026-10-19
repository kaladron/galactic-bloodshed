// SPDX-License-Identifier: Apache-2.0

/// \file unpledge.cc
/// \brief Withdraw pledge to an alliance block.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {
/* declare that you wish to withdraw from the alliance block */
bool unpledge(const command_t& argv, GameObj& g) {
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

  GB::presentation::UnpledgeResult result{
      .pledger_id = Playernum,
      .pledger_name = g.race->name,
      .block_id = n,
      .taunt = (uniform_rand(1, 20) == 1),
  };

  g.entity_manager.mutate_block(n, [&](Block& b) {
    b.unpledge(Playernum);
    result.block_name = b.name();
  });

  warn_race(g.session_registry, g.entity_manager, n,
            GB::presentation::render_unpledge_block_notification(result));
  warn_race(g.session_registry, g.entity_manager, Playernum,
            GB::presentation::render_unpledge_self_notification(result));
  post(g.entity_manager,
       GB::presentation::render_unpledge_declaration_news(result),
       NewsType::DECLARATION);
  compute_power_blocks(g.entity_manager);

  g.present(result);
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