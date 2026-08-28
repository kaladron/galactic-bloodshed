// SPDX-License-Identifier: Apache-2.0

/// \file highlight.cc
/// \brief Toggle highlight option on a player.

module;

import std;
import gb.entities;
import gb.services;

module commands;

namespace GB::commands {

bool highlight(const command_t& argv, GameObj& g) {
  player_t n = get_player(g.entity_manager, argv[1]);
  if (n.value == 0) {
    g.out << "No such player.\n";
    return false;
  }

  g.entity_manager.mutate_race(g.player(), [&](Race& race) {
    race.governor[g.governor().value].toggle.highlight = n;
  });
  return true;
}

const CommandDescriptor highlight_cmd{
    .name = "highlight",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "highlight <player>",
    .description = "Set the player to highlight in maps and displays",
    .handler = &highlight,
};

}  // namespace GB::commands
