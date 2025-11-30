// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void detonate(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (argv.size() < 3) {
    std::string msg = "Syntax: '" + argv[0] + " <mine>'\n";
    notify(Playernum, Governor, msg);
    return;
  }

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
  }
}
}  // namespace GB::commands
