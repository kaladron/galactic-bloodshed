// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void personal(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  if (g.governor() != 0) {
    g.out << "Only the leader can do this.\n";
    return;
  }
  auto race = g.entity_manager.get_race(Playernum);
  race->info = message;
}
}  // namespace GB::commands
