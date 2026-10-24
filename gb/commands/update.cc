// SPDX-License-Identifier: Apache-2.0

/// \file update.cc
/// \brief Trigger a game update (deity only).

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import gb.turn;
import std;

module commands;

namespace GB::commands {

bool update(const command_t& argv, GameObj& g) {
  if (argv.size() > 1) {
    g.out << "Syntax: @@update\n";
    return false;
  }
  g.session_registry.flush_all();
  do_update(g.entity_manager, g.session_registry, true);
  g.present(GB::presentation::UpdateResult{});
  return true;
}

const CommandDescriptor update_cmd{
    .name = "@@update",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "@@update",
    .description = "Trigger a game update (deity only)",
    .handler = &update,
    .transactional = false,
};

}  // namespace GB::commands
