// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

#include "gb/tweakables.h"

namespace GB::commands {
void motto(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): ap_t APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  ss_message << std::ends;
  std::string message = ss_message.str();

  if (Governor) {
    g.out << "You are not authorized to do this.\n";
    return;
  }
  strncpy(Blocks[Playernum - 1].motto, message.c_str(), MOTTOSIZE - 1);
  Putblock(Blocks);
  g.out << "Done.\n";
}
}  // namespace GB::commands
