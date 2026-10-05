// SPDX-License-Identifier: Apache-2.0

/// \file detonate.cc
/// \brief Detonate space mine(s) command.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool detonate(const command_t& argv, GameObj& g) {
  bool any_detonated = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    Ship& s = *ship_handle;

    if (s.type() != ShipType::STYPE_MINE) {
      g.out << "That is not a mine.\n";
      continue;
    }
    if (!s.on()) {
      g.out << "The mine is not activated.\n";
      continue;
    }
    if (s.docked() || s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      g.out << "The mine is docked or landed.\n";
      continue;
    }

    domine(s, /*detonate=*/true, g.entity_manager);
    any_detonated = true;
  }

  return any_detonated;
}

const CommandDescriptor detonate_cmd{
    .name = "detonate",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "detonate <mine>",
    .description = "Detonate space mine(s)",
    .handler = &detonate,
};

}  // namespace GB::commands
