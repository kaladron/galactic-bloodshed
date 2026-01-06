// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void quit([[maybe_unused]] const command_t& argv, GameObj& g) {
  g.out << "Goodbye!\n";
  g.set_disconnect_requested(true);
}
}  // namespace GB::commands
