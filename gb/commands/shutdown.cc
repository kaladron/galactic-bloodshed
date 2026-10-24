// SPDX-License-Identifier: Apache-2.0

/// \file shutdown.cc
/// \brief Initiate emergency server shutdown.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool shutdown(const command_t& argv, GameObj& g) {
  if (argv.size() > 1) {
    g.out << "Syntax: @@shutdown\n";
    return false;
  }
  g.set_shutdown_requested(true);
  g.present(GB::presentation::ShutdownResult{});
  return true;
}

const CommandDescriptor shutdown_cmd{
    .name = "@@shutdown",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "@@shutdown",
    .description = "Initiate emergency server shutdown (deity only)",
    .handler = &shutdown,
};

}  // namespace GB::commands
