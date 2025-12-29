// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void pay(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  // TODO(jeffbailey): ap_t APcount = 0;
  int who;
  int amount;

  if (!(who = get_player(g.entity_manager, argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor) {
    g.out << "You are not authorized to do that.\n";
    return;
  }

  // g.race is already set by process_command(), no need to check
  auto race_handle = g.entity_manager.get_race(Playernum);
  auto alien_handle = g.entity_manager.get_race(who);
  if (!alien_handle.get()) {
    g.out << "Alien race not found.\n";
    return;
  }
  auto& race = *race_handle;
  auto& alien = *alien_handle;

  amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to give a player a positive amount of money.\n";
    return;
  }
  if (g.race->Guest) {
    g.out << "Nice try. Your attempt has been duly noted.\n";
    return;
  }
  if (race.governor[Governor].money < amount) {
    g.out << "You don't have that much money to give!\n";
    return;
  }

  race.governor[Governor].money -= amount;
  alien.governor[0].money += amount;
  warn(who, 0,
       std::format("{} [{}] payed you {}.\n", race.name, Playernum, amount));
  g.out << std::format("{} payed to {} [{}].\n", amount, alien.name, who);

  post(g.entity_manager,
       std::format("{} [{}] pays {} [{}].\n", race.name, Playernum, alien.name,
                   who),
       NewsType::TRANSFER);
}
}  // namespace GB::commands