// SPDX-License-Identifier: Apache-2.0

/// \file center.cc
/// \brief Center scope on a star system.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import std;

module commands;

namespace GB::commands {
bool center(const command_t& argv, GameObj& g) {
  if (argv.size() != 2) {
    g.out << "center: which star?\n";
    return false;
  }
  auto where =
      Place::resolve(g.entity_manager, g.scope_context(), argv[1], true);

  if (!where) {
    g.out << format_place_error(where.error());
    g.out << "center: bad scope.\n";
    return false;
  }
  if (where->level == ScopeLevel::LEVEL_SHIP) {
    g.out << "CHEATER!!!\n";
    return false;
  }
  if (where->level == ScopeLevel::LEVEL_UNIV) {
    g.out << "center: bad scope.\n";
    return false;
  }
  const auto& star = *g.entity_manager.peek_star(where->snum);
  g.set_universe_center(star.coordinates());
  g.present(GB::presentation::CenterResult{
      .snum = where->snum,
      .star_name = star.get_name(),
      .universe_center = g.universe_center(),
  });
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
