// SPDX-License-Identifier: Apache-2.0

/// \file shutdown.cc
/// \brief Initiate emergency server shutdown.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool shutdown(const command_t&, GameObj& g) {
  g.set_shutdown_requested(true);
  g.out << "Doing shutdown.\n";
  return true;
}

const CommandDescriptor shutdown_cmd{
    .name = "@@shutdown",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .description = "Initiate emergency server shutdown (deity only)",
    .handler = &shutdown,
};

}  // namespace GB::commands
