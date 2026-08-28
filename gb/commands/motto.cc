// SPDX-License-Identifier: Apache-2.0

/// \file motto.cc
/// \brief Set alliance block motto.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool motto(const command_t& argv, GameObj& g) {
  // Concatenate all arguments after command name into motto string
  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  try {
    g.entity_manager.mutate_block(g.player().value,
                                  [&](struct block& b) { b.motto = message; });
  } catch (const EntityNotFoundError&) {
    g.out << "Block not found.\n";
    return false;
  }

  g.out << "Done.\n";
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
