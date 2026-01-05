// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void purge([[maybe_unused]] const command_t& argv, GameObj& g) {
  if (!g.race->God) {
    g.out << "Only deity can use this command.\n";
    return;
  }

  ::purge(g.entity_manager);
  g.out << "Purged all news.\n";
}
}  // namespace GB::commands
