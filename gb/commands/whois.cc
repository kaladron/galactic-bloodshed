// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace GB::commands {
void whois(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;

  if (argv.size() <= 1) {
    whois({"whois", std::to_string(Playernum)}, g);
    return;
  }
  auto numraces = Num_races;

  for (size_t i = 1; i <= argv.size() - 1; i++) {
    auto j = std::stoi(argv[i]);
    if (!(j < 1 || j > numraces)) {
      auto &race = races[j - 1];
      if (j == Playernum)
        sprintf(buf, "[%2d, %d] %s \"%s\"\n", j, Governor, race.name,
                race.governor[Governor].name);
      else
        sprintf(buf, "[%2d] %s\n", j, race.name);
    } else {
      sprintf(buf, "Identify: Invalid player number #%d. Try again.\n", j);
    }
    notify(Playernum, Governor, buf);
  }
}
}  // namespace GB::commands