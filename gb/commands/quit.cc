// SPDX-License-Identifier: Apache-2.0

/// \file quit.cc
/// \brief Disconnect from the game server.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool quit(const command_t&, GameObj& g) {
  g.out << "Goodbye!\n";
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
