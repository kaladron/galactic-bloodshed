// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void read_messages(const command_t& argv, GameObj& g) {
  // TODO(jeffbailey): ap_t APcount = 0;
  if (argv.size() == 1 || argv[1] == "telegram")
    teleg_read(g);
  else if (argv[1] == "news") {
    g.out << CUTE_MESSAGE;
    g.out << "\n----------        Declarations        ----------\n";
    news_read(NewsType::DECLARATION, g);
    g.out << "\n----------           Combat           ----------\n";
    news_read(NewsType::COMBAT, g);
    g.out << "\n----------          Business          ----------\n";
    news_read(NewsType::TRANSFER, g);
    g.out << "\n----------          Bulletins         ----------\n";
    news_read(NewsType::ANNOUNCE, g);
  } else
    g.out << "Read what?\n";
}
}  // namespace GB::commands
