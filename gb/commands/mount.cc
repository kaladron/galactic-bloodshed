// SPDX-License-Identifier: Apache-2.0

/// \file mount.cc
/// \brief Functions for mounting and dismounting crystals in ships.

module;

import gb.entities;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool mount(const command_t& argv, GameObj& g) {
  const governor_t Governor = g.governor();
  bool mnt;
  mnt = argv[0] == "mount";
  bool success = false;

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& ship = *ship_handle;

    if (!ship_matches_filter(argv[1], ship)) continue;
    if (!authorized(Governor, ship)) continue;

    if (!ship.mount()) {
      g.out << "This ship is not equipped with a crystal mount.\n";
      continue;
    }
    if (ship.mounted() && mnt) {
      g.out << "You already have a crystal mounted.\n";
      continue;
    }
    if (!ship.mounted() && !mnt) {
      g.out << "You don't have a crystal mounted.\n";
      continue;
    }
    if (!ship.mounted() && mnt) {
      if (!ship.crystals()) {
        g.out << "You have no crystals on board.\n";
        continue;
      }
      ship.mounted() = 1;
      ship.crystals()--;
      g.out << "Mounted.\n";
      success = true;
    } else if (ship.mounted() && !mnt) {
      if (ship.crystals() >= MAX_CRYSTALS) {
        g.out << "You can't dismount the crystal. Max "
                 "allowed already on board.\n";
        continue;
      }
      ship.mounted() = 0;
      ship.crystals()++;
      g.out << "Dismounted.\n";
      if (ship.hyper_drive().charge > 0) {
        ship.hyper_drive().charge = 0;
        g.out << "Discharged.\n";
      }
      if (ship.laser() && ship.fire_laser()) {
        ship.fire_laser() = 0;
        g.out << "Laser deactivated.\n";
      }
      success = true;
    } else {
      g.out << "Weird error in 'mount'.\n";
      continue;
    }
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

const CommandDescriptor dismount_cmd{
    .name = "dismount",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "dismount <ship>",
    .description = "Dismount a crystal from a ship's hyperdrive",
    .handler = &mount,
};

}  // namespace GB::commands
