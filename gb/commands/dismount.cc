// SPDX-License-Identifier: Apache-2.0

/// \file dismount.cc
/// \brief Dismount a crystal from a ship's hyperdrive.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool dismount(const command_t& argv, GameObj& g) {
  bool success = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = dismount_ship_crystal(*ship_handle);
    if (!res) {
      g.out << GB::presentation::render_dismount_crystal_error(res.error());
      continue;
    }
    g.present(*res);
    success = true;
  }
  return success;
}

const CommandDescriptor dismount_cmd{
    .name = "dismount",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "dismount <ship>",
    .description = "Dismount a crystal from a ship's hyperdrive",
    .handler = &dismount,
};

}  // namespace GB::commands
