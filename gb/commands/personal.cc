// SPDX-License-Identifier: Apache-2.0

/// \file personal.cc
/// \brief Set personal description info for your race.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool personal(const command_t& argv, GameObj& g) {
  const player_t playernum = g.player();
  auto message = argv | std::views::drop(1) | std::views::join_with(' ') |
                 std::ranges::to<std::string>();

  g.entity_manager.mutate_race(playernum,
                               [&](Race& race) { race.info = message; });
  g.present(GB::presentation::PersonalResult{
      .player = playernum,
      .info = std::move(message),
  });
  return true;
}

const CommandDescriptor personal_cmd{
    .name = "personal",
    .roles = {.leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "personal <info text>",
    .description = "Set personal information description for your race",
    .handler = &personal,
};

}  // namespace GB::commands
