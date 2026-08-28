// SPDX-License-Identifier: Apache-2.0

/// \file read_messages.cc
/// \brief Read telegrams, news bulletins, and announcements.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool read_messages(const command_t& argv, GameObj& g) {
  if (argv.size() == 1 || argv[1] == "telegram") {
    teleg_read(g);
    return true;
  }
  if (argv[1] == "news") {
    g.out << CUTE_MESSAGE;
    g.out << "\n----------        Declarations        ----------\n";
    news_read(NewsType::DECLARATION, g);
    g.out << "\n----------           Combat           ----------\n";
    news_read(NewsType::COMBAT, g);
    g.out << "\n----------          Business          ----------\n";
    news_read(NewsType::TRANSFER, g);
    g.out << "\n----------          Bulletins         ----------\n";
    news_read(NewsType::ANNOUNCE, g);
    return true;
  }
  g.out << "Read what?\n";
  return false;
}

const CommandDescriptor read_cmd{
    .name = "read",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "read [telegram|news]",
    .description = "Read telegram messages or public news bulletins",
    .handler = &read_messages,
};

}  // namespace GB::commands
