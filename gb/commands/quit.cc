// SPDX-License-Identifier: Apache-2.0

/// \file quit.cc
/// \brief Disconnect from the game server.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool quit(const command_t& argv, GameObj& g) {
  if (argv.size() > 1) {
    g.out << "Syntax: quit\n";
    return false;
  }
  g.present(GB::presentation::QuitResult{});
  g.set_disconnect_requested(true);
  return true;
}

const CommandDescriptor quit_cmd{
    .name = "quit",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "quit",
    .description = "Disconnect from the game server",
    .handler = &quit,
};

}  // namespace GB::commands
