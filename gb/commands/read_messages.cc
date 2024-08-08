// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/files.h"
#include "gb/tele.h"
module commands;

namespace GB::commands {
void read_messages(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): ap_t APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (argv.size() == 1 || argv[1] == "telegram")
    teleg_read(g);
  else if (argv[1] == "news") {
    notify(Playernum, Governor, CUTE_MESSAGE);
    notify(Playernum, Governor,
           "\n----------        Declarations        ----------\n");
    news_read(Playernum, Governor, DECLARATION);
    notify(Playernum, Governor,
           "\n----------           Combat           ----------\n");
    news_read(Playernum, Governor, COMBAT);
    notify(Playernum, Governor,
           "\n----------          Business          ----------\n");
    news_read(Playernum, Governor, TRANSFER);
    notify(Playernum, Governor,
           "\n----------          Bulletins         ----------\n");
    news_read(Playernum, Governor, ANNOUNCE);
  } else
    g.out << "Read what?\n";
}
}  // namespace GB::commands
