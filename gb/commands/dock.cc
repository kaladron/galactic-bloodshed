// SPDX-License-Identifier: Apache-2.0

/// \file dock.cc
/// \brief Peacefully dock a ship with another ship.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {

bool dock(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Dock with what?\n";
    return false;
  }

  bool any_docked = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    const auto target_id = string_to_shipnum(argv[2]);
    if (!target_id) {
      g.out << "Invalid ship number.\n";
      return any_docked;
    }

    auto res = dock_single_ship(g.entity_manager, *ship_handle, *target_id,
                                g.player(), g.governor(), g.god());
    if (!res) {
      g.out << GB::presentation::render_peaceful_dock_error(res.error());
      if (res.error().abort_loop) {
        return any_docked;
      }
      continue;
    }

    g.present(*res);
    any_docked = true;
  }

  return any_docked;
}

const CommandDescriptor dock_cmd{
    .name = "dock",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "dock <ship> <target_ship>",
    .description = "Dock a ship with another ship",
    .handler = &dock,
};

}  // namespace GB::commands
