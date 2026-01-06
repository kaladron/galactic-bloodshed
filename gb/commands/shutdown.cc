// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void shutdown([[maybe_unused]] const command_t& argv, GameObj& g) {
  if (!g.race->God) {
    g.out << "Only deity can use this command.\n";
    return;
  }

  g.set_shutdown_requested(true);
  g.out << "Doing shutdown.\n";
}
}  // namespace GB::commands
