// SPDX-License-Identifier: Apache-2.0

/// \file pay.cc
/// \brief Pay funds from treasury to another race.

module;

import gblib;
import notification;
import session;
import std;
import scnlib;

module commands;

namespace GB::commands {

bool pay(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  player_t who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }

  auto alien_handle = [&]() -> std::optional<EntityHandle<Race>> {
    try {
      return g.entity_manager.get_race(who);
    } catch (const EntityNotFoundError&) {
      return std::nullopt;
    }
  }();
  if (!alien_handle) {
    g.out << "Alien race not found.\n";
    return false;
  }
  auto race_handle = g.entity_manager.get_race(Playernum);
  auto& race = *race_handle;
  auto& alien = **alien_handle;

  auto parsed_amount = scn::scan<int>(argv[2], "{}");
  if (!parsed_amount) {
    g.out << "Invalid amount.\n";
    return false;
  }
  int amount = parsed_amount->value();
  if (amount < 0) {
    g.out << "You have to give a player a positive amount of money.\n";
    return false;
  }
  if (race.governor[g.governor().value].money < amount) {
    g.out << "You don't have that much money to give!\n";
    return false;
  }

  race.governor[g.governor().value].money -= amount;
  alien.governor[0].money += amount;
  warn_player(
      g.session_registry, g.entity_manager, who, 0,
      std::format("{} [{}] payed you {}.\n", race.name, Playernum, amount));
  g.out << std::format("{} payed to {} [{}].\n", amount, alien.name, who);

  post(g.entity_manager,
       std::format("{} [{}] pays {} [{}].\n", race.name, Playernum, alien.name,
                   who),
       NewsType::TRANSFER);
  return true;
}

const CommandDescriptor pay_cmd{
    .name = "pay",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "pay <race> <amount>",
    .description =
        "Transfer money from your treasury to another race's treasury",
    .handler = &pay,
};

}  // namespace GB::commands