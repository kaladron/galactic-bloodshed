// SPDX-License-Identifier: Apache-2.0

/// \file scrap.cc
/// \brief Scrap ships for raw materials.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {

bool scrap(const command_t& argv, GameObj& g) {
  bool any_scrapped = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = scrap_single_ship(g.entity_manager, *ship_handle, *g.race);
    if (!res) {
      g.out << GB::presentation::render_scrap_error(res.error());
      continue;
    }
    g.present(*res);
    any_scrapped = true;
  }

  return any_scrapped;
}

const CommandDescriptor scrap_cmd{
    .name = "scrap",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "scrap <ship>",
    .description = "Scrap a ship to reclaim resources, fuel, and crew",
    .handler = &scrap,
};

}  // namespace GB::commands
