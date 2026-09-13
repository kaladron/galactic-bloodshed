// SPDX-License-Identifier: Apache-2.0

/// \file center.cc
/// \brief Center scope on a star system.

module;

import std;
import gb.entities;
import gb.services;

module commands;

namespace GB::commands {
bool center(const command_t& argv, GameObj& g) {
  if (argv.size() != 2) {
    g.out << "center: which star?\n";
    return false;
  }
  Place where{g, argv[1], true};

  if (where.err) {
    g.out << "center: bad scope.\n";
    return false;
  }
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    g.out << "CHEATER!!!\n";
    return false;
  }
  if (where.level == ScopeLevel::LEVEL_UNIV) {
    g.out << "center: bad scope.\n";
    return false;
  }
  const auto& star = *g.entity_manager.peek_star(where.snum);
  g.set_universe_center(star.coordinates());
  return true;
}

const CommandDescriptor center_cmd{
    .name = "center",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "center <star>",
    .description = "Change global map centerpoint to another star",
    .handler = &center,
};

}  // namespace GB::commands
