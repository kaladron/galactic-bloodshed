// SPDX-License-Identifier: Apache-2.0

/// \file emulate.cc
/// \brief Emulate another player and governor (deity only).

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool emulate(const command_t& argv, GameObj& g) {
  if (argv.size() > 3) {
    g.out << "Syntax: emulate <player> <governor>\n";
    return false;
  }

  auto p_res = scn::scan<player_t::value_type>(argv[1], "{}");
  auto g_res = scn::scan<governor_t::value_type>(argv[2], "{}");
  if (!p_res || !p_res->range().empty() || !g_res || !g_res->range().empty()) {
    g.out << "Invalid player or governor number.\n";
    return false;
  }
  const player_t new_player = p_res->value();
  const governor_t new_gov = g_res->value();

  const Race* race = nullptr;
  try {
    race = g.entity_manager.peek_race(new_player);
  } catch (const EntityNotFoundError&) {
    g.out << std::format("Player {} does not exist.\n", new_player);
    return false;
  }
  if (new_gov < 1) {
    g.out << std::format("Invalid governor {}. Must be >= 1.\n", new_gov);
    return false;
  }
  if (!race->has_governor(new_gov)) {
    g.out << std::format("Governor {} is not active.\n", new_gov);
    return false;
  }

  // Switch to new player/governor
  g.set_player(new_player);
  g.set_governor(new_gov);
  g.set_god(false);  // When emulating, act as normal player
  g.race = race;

  g.present(GB::presentation::EmulateResult{
      .player = new_player,
      .governor = new_gov,
      .race_name = race->name,
      .governor_name = race->governor(new_gov).name,
  });
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
