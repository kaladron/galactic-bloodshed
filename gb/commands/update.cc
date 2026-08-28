// SPDX-License-Identifier: Apache-2.0

/// \file update.cc
/// \brief Trigger a game update (deity only).

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool update(const command_t&, GameObj& g) {
  g.out << "Starting update...\n";
  g.session_registry.flush_all();
  do_update(g.entity_manager, g.session_registry, true);
  g.out << "Update completed.\n";
  return true;
}

const CommandDescriptor update_cmd{
    .name = "@@update",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .description = "Trigger a game update (deity only)",
    .handler = &update,
};

}  // namespace GB::commands
