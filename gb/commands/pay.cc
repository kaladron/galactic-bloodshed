// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/tele.h"

module commands;

namespace GB::commands {
void pay(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  int who;
  int amount;

  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor) {
    g.out << "You are not authorized to do that.\n";
    return;
  }
  auto &race = races[Playernum - 1];
  auto &alien = races[who - 1];

  amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to give a player a positive amount of money.\n";
    return;
  }
  if (race.Guest) {
    g.out << "Nice try. Your attempt has been duly noted.\n";
    return;
  }
  if (race.governor[Governor].money < amount) {
    g.out << "You don't have that much money to give!\n";
    return;
  }

  race.governor[Governor].money -= amount;
  alien.governor[0].money += amount;
  sprintf(buf, "%s [%d] payed you %d.\n", race.name, Playernum, amount);
  warn(who, 0, buf);
  sprintf(buf, "%d payed to %s [%d].\n", amount, alien.name, who);
  notify(Playernum, Governor, buf);

  sprintf(buf, "%s [%d] pays %s [%d].\n", race.name, Playernum, alien.name,
          who);
  post(buf, TRANSFER);

  putrace(alien);
  putrace(race);
}
}  // namespace GB::commands