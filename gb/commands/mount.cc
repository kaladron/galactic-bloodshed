// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void mount(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  bool mnt;
  mnt = argv[0] == "mount";

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
  for (auto ship_handle : ships) {
    Ship& ship = *ship_handle;

    if (!ship_matches_filter(argv[1], ship)) continue;
    if (!authorized(Governor, ship)) continue;

    if (!ship.mount()) {
      notify(Playernum, Governor,
             "This ship is not equipped with a crystal mount.\n");
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
    } else if (ship.mounted() && !mnt) {
      if (ship.crystals() == max_crystals(ship)) {
        notify(Playernum, Governor,
               "You can't dismount the crystal. Max "
               "allowed already on board.\n");
        continue;
      }
      ship.mounted() = 0;
      ship.crystals()++;
      g.out << "Dismounted.\n";
      if (ship.hyper_drive().charge || ship.hyper_drive().ready) {
        ship.hyper_drive().charge = 0;
        ship.hyper_drive().ready = 0;
        g.out << "Discharged.\n";
      }
      if (ship.laser() && ship.fire_laser()) {
        ship.fire_laser() = 0;
        g.out << "Laser deactivated.\n";
      }
    } else {
      g.out << "Weird error in 'mount'.\n";
      continue;
    }
  }
}
}  // namespace GB::commands
