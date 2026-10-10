// SPDX-License-Identifier: Apache-2.0

/// \file mount.cc
/// \brief Mount a crystal into a ship's hyperdrive.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.ship_ops;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool mount(const command_t& argv, GameObj& g) {
  bool success = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = mount_ship_crystal(*ship_handle);
    if (!res) {
      g.out << GB::presentation::render_mount_crystal_error(res.error());
      continue;
    }
    g.present(*res);
    success = true;
  }
  return success;
}

const CommandDescriptor mount_cmd{
    .name = "mount",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "mount <ship>",
    .description = "Mount a crystal into a ship's hyperdrive",
    .handler = &mount,
};

}  // namespace GB::commands
