// SPDX-License-Identifier: Apache-2.0

// declare.c -- declare alliance, neutrality, war, the basic thing.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
/* invite people to join your alliance block */
void invite(const command_t& argv, GameObj& g) {
  bool mode = argv[0] == "invite" ? true : false;

  player_t n;

  if (g.governor) {
    g.out << "Only leaders may invite.\n";
    return;
  }
  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (n == g.player) {
    g.out << "Not needed, you are the leader.\n";
    return;
  }
  auto& race = races[g.player - 1];
  auto& alien = races[n - 1];
  std::string buf;
  if (mode) {
    setbit(Blocks[g.player - 1].invite, n);
    buf = std::format("{} [{}] has invited you to join {}\n", race.name,
                      g.player, Blocks[g.player - 1].name);
    warn_race(n, buf);
    buf = std::format("{} [{}] has been invited to join {} [{}]\n", alien.name,
                      n, Blocks[g.player - 1].name, g.player);
    warn_race(g.player, buf);
  } else {
    clrbit(Blocks[g.player - 1].invite, n);
    buf = std::format("You have been blackballed from {} [{}]\n",
                      Blocks[g.player - 1].name, g.player);
    warn_race(n, buf);
    buf = std::format("{} [{}] has been blackballed from {} [{}]\n", alien.name,
                      n, Blocks[g.player - 1].name, g.player);
    warn_race(g.player, buf);
  }
  post(buf, NewsType::DECLARATION);

  Putblock(Blocks);
}
}  // namespace GB::commands
