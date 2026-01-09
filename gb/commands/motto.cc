// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void motto(const command_t& argv, GameObj& g) {
  if (g.governor() != 0) {
    g.out << "You are not authorized to do this.\n";
    return;
  }

  // Concatenate all arguments after command name into motto string
  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  // Get block for modification (RAII auto-saves on scope exit)
  auto block_handle = g.entity_manager.get_block(g.player());
  if (!block_handle.get()) {
    g.out << "Block not found.\n";
    return;
  }

  auto& block = *block_handle;
  block.motto = message;

  g.out << "Done.\n";
}
}  // namespace GB::commands
