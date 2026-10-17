// SPDX-License-Identifier: Apache-2.0

/// \file uninvite.cc
/// \brief Blackball/uninvite players from an alliance block.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool uninvite(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const player_t n = get_player(g.entity_manager, argv[1]);
  if (n == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return false;
  }

  const auto& alien = *g.entity_manager.peek_race(n);
  GB::presentation::UninviteResult result{
      .host_id = Playernum,
      .target_id = n,
      .target_name = alien.name,
  };

  g.entity_manager.mutate_block(Playernum, [&](Block& b) {
    b.uninvite(n);
    result.block_name = b.name();
  });

  warn_race(g.session_registry, g.entity_manager, n,
            GB::presentation::render_uninvite_target_notification(result));
  const std::string decl =
      GB::presentation::render_uninvite_declaration_news(result);
  warn_race(g.session_registry, g.entity_manager, Playernum, decl);
  post(g.entity_manager, decl, NewsType::DECLARATION);
  compute_power_blocks(g.entity_manager);
  g.present(result);
  return true;
}

const CommandDescriptor uninvite_cmd{
    .name = "uninvite",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "uninvite <player>",
    .description = "Blackball/uninvite a player from your alliance block",
    .handler = &uninvite,
};

}  // namespace GB::commands
