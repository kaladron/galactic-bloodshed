// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/files_shl.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"

module commands;

void mount(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  bool mnt;
  mnt = argv[0] == "mount";

  Ship *ship;
  shipnum_t shipno;
  shipnum_t nextshipno;

  nextshipno = start_shiplist(g, argv[1]);
  while ((shipno = do_shiplist(&ship, &nextshipno)))
    if (in_list(Playernum, argv[1], *ship, &nextshipno) &&
        authorized(Governor, *ship)) {
      if (!ship->mount) {
        notify(Playernum, Governor,
               "This ship is not equipped with a crystal mount.\n");
        free(ship);
        continue;
      }
      if (ship->mounted && mnt) {
        g.out << "You already have a crystal mounted.\n";
        free(ship);
        continue;
      }
      if (!ship->mounted && !mnt) {
        g.out << "You don't have a crystal mounted.\n";
        free(ship);
        continue;
      }
      if (!ship->mounted && mnt) {
        if (!ship->crystals) {
          g.out << "You have no crystals on board.\n";
          free(ship);
          continue;
        }
        ship->mounted = 1;
        ship->crystals--;
        g.out << "Mounted.\n";
      } else if (ship->mounted && !mnt) {
        if (ship->crystals == max_crystals(*ship)) {
          notify(Playernum, Governor,
                 "You can't dismount the crystal. Max "
                 "allowed already on board.\n");
          free(ship);
          continue;
        }
        ship->mounted = 0;
        ship->crystals++;
        g.out << "Dismounted.\n";
        if (ship->hyper_drive.charge || ship->hyper_drive.ready) {
          ship->hyper_drive.charge = 0;
          ship->hyper_drive.ready = 0;
          g.out << "Discharged.\n";
        }
        if (ship->laser && ship->fire_laser) {
          ship->fire_laser = 0;
          g.out << "Laser deactivated.\n";
        }
      } else {
        g.out << "Weird error in 'mount'.\n";
        free(ship);
        continue;
      }
      putship(ship);
      free(ship);
    } else
      free(ship);
}
