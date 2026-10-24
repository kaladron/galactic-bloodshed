// SPDX-License-Identifier: Apache-2.0

/// \file motto.cc
/// \brief Set alliance block motto.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool motto(const command_t& argv, GameObj& g) {
  // Concatenate all arguments after command name into motto string
  auto message = argv | std::views::drop(1) | std::views::join_with(' ') |
                 std::ranges::to<std::string>();

  g.entity_manager.mutate_block(g.player(),
                                [&](Block& b) { b.motto() = message; });
  g.present(GB::presentation::MottoResult{
      .block_id = g.player(),
      .motto = std::move(message),
  });
  return true;
}

const CommandDescriptor motto_cmd{
    .name = "motto",
    .roles = {.leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "motto <motto text>",
    .description = "Set the motto for your alliance block",
    .handler = &motto,
};

}  // namespace GB::commands
