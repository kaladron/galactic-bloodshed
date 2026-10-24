// SPDX-License-Identifier: Apache-2.0

/// \file post.cc
/// \brief Post public announcement bulletins to Galactic News.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool post_bulletin(const command_t& argv, GameObj& g) {
  const auto sender =
      GB::presentation::format_sender_tag(*g.race, g.player(), g.governor());
  const auto body = argv | std::views::drop(1) | std::views::join_with(' ') |
                    std::ranges::to<std::string>();
  post(g.entity_manager, std::format("{}: {}\n", sender, body),
       NewsType::ANNOUNCE);
  g.present(GB::presentation::PostResult{
      .sender = sender,
      .message = body,
  });
  return true;
}

const CommandDescriptor post_cmd{
    .name = "post",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "post <message>",
    .description = "Post a public announcement bulletin to all players",
    .handler = &post_bulletin,
};

}  // namespace GB::commands
