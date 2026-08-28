// SPDX-License-Identifier: Apache-2.0

/// \file emulate.cc
/// \brief Emulate another player and governor (deity only).

module;

import gb.entities;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {

bool emulate(const command_t& argv, GameObj& g) {
  player_t new_player = 0;
  governor_t new_gov = 0;

  auto [ptr1, ec1] = std::from_chars(
      argv[1].data(), argv[1].data() + argv[1].size(), new_player.value);
  auto [ptr2, ec2] = std::from_chars(
      argv[2].data(), argv[2].data() + argv[2].size(), new_gov.value);
  if (ec1 != std::errc{} || ec2 != std::errc{}) {
    g.out << "Invalid player or governor number.\n";
    return false;
  }

  const Race* race = nullptr;
  try {
    race = g.entity_manager.peek_race(new_player);
  } catch (const EntityNotFoundError&) {
    g.out << std::format("Player {} does not exist.\n", new_player);
    return false;
  }
  if (!race) {
    g.out << std::format("Player {} does not exist.\n", new_player);
    return false;
  }
  if (new_gov < 0 || new_gov > MAXGOVERNORS) {
    g.out << std::format("Invalid governor {}. Must be 0-{}.\n", new_gov,
                         MAXGOVERNORS);
    return false;
  }
  if (!race->governor[new_gov.value].active) {
    g.out << std::format("Governor {} is not active.\n", new_gov);
    return false;
  }

  // Switch to new player/governor
  g.set_player(new_player);
  g.set_governor(new_gov);
  g.set_god(false);  // When emulating, act as normal player
  g.race = race;

  g.out << std::format("Emulating {} \"{}\" [{},{}]\n", race->name,
                       race->governor[new_gov.value].name, new_player, new_gov);
  return true;
}

const CommandDescriptor emulate_cmd{
    .name = "emulate",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "emulate <player> <governor>",
    .description = "Emulate another player and governor (deity only)",
    .handler = &emulate,
};

}  // namespace GB::commands
