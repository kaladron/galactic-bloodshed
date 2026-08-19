// SPDX-License-Identifier: Apache-2.0

/// \file detonate.cc
/// \brief Detonate space mine(s) command.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool detonate(const command_t& argv, GameObj& g) {
  const governor_t Governor = g.governor();
  bool any_detonated = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

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

    domine(s, 1, g.entity_manager);
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
