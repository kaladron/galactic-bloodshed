// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void personal(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  ss_message << std::ends;
  std::string message = ss_message.str();

  if (g.governor != 0) {
    g.out << "Only the leader can do this.\n";
    return;
  }
  auto race = races[Playernum - 1];
  strncpy(race.info, message.c_str(), PERSONALSIZE - 1);
  putrace(race);
}
}  // namespace GB::commands
