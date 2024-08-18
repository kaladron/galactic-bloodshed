// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/buffers.h"

module commands;

namespace GB::commands {
/* upgrade ship characteristics */
void upgrade(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  // TODO(jeffbailey): Fix unused ap_t APcount = 1;
  int value;
  int oldcost;
  int newcost;
  int netcost;
  double complex;

  if (g.level != ScopeLevel::LEVEL_SHIP) {
    g.out << "You have to change scope to the ship you wish to upgrade.\n";
    return;
  }
  auto dirship = getship(g.shipno);
  if (!dirship) {
    g.out << "Illegal dir value.\n";
    return;
  }
  if (testship(*dirship, Playernum, Governor)) {
    return;
  }
  if (dirship->damage) {
    g.out << "You cannot upgrade damaged ships.\n";
    return;
  }
  if (dirship->type == ShipType::OTYPE_FACTORY) {
    g.out << "You can't upgrade factories.\n";
    return;
  }

  auto &race = races[Playernum - 1];
  auto ship = *dirship;

  if (argv.size() == 3)
    value = std::stoi(argv[2]);
  else
    value = 0;

  if (value < 0) {
    g.out << "That's a ridiculous setting.\n";
    return;
  }

  if (!Shipdata[dirship->build_type][ABIL_MOD]) {
    g.out << "This ship cannot be upgraded.\n";
    return;
  }

  if (argv[1] == "armor") {
    ship.armor = MAX(dirship->armor, MIN(value, 100));
  } else if (argv[1] == "crew" && Shipdata[dirship->build_type][ABIL_MAXCREW]) {
    ship.max_crew = MAX(dirship->max_crew, MIN(value, 10000));
  } else if (argv[1] == "cargo" && Shipdata[dirship->build_type][ABIL_CARGO]) {
    ship.max_resource = MAX(dirship->max_resource, MIN(value, 10000));
  } else if (argv[1] == "hanger" &&
             Shipdata[dirship->build_type][ABIL_HANGER]) {
    ship.max_hanger = MAX(dirship->max_hanger, MIN(value, 10000));
  } else if (argv[1] == "fuel" && Shipdata[dirship->build_type][ABIL_FUELCAP]) {
    ship.max_fuel = MAX(dirship->max_fuel, MIN(value, 10000));
  } else if (argv[1] == "mount" && Shipdata[dirship->build_type][ABIL_MOUNT] &&
             !dirship->mount) {
    if (!Crystal(race)) {
      g.out << "Your race does not now how to utilize crystal power yet.\n";
      return;
    }
    ship.mount = !ship.mount;
  } else if (argv[1] == "destruct" &&
             Shipdata[dirship->build_type][ABIL_DESTCAP]) {
    ship.max_destruct = MAX(dirship->max_destruct, MIN(value, 10000));
  } else if (argv[1] == "speed" && Shipdata[dirship->build_type][ABIL_SPEED]) {
    ship.max_speed = MAX(dirship->max_speed, MAX(1, MIN(value, 9)));
  } else if (argv[1] == "hyperdrive" &&
             Shipdata[dirship->build_type][ABIL_JUMP] &&
             !dirship->hyper_drive.has && Hyper_drive(race)) {
    ship.hyper_drive.has = 1;
  } else if (argv[1] == "primary" &&
             Shipdata[dirship->build_type][ABIL_PRIMARY]) {
    if (argv[2] == "strength") {
      if (ship.primtype == GTYPE_NONE) {
        g.out << "No caliber defined.\n";
        return;
      }
      ship.primary = std::stoi(argv[3]);
      ship.primary = MAX(ship.primary, dirship->primary);
    } else if (argv[2] == "caliber") {
      if (argv[3] == "light")
        ship.primtype = MAX(GTYPE_LIGHT, dirship->primtype);
      else if (argv[3] == "medium")
        ship.primtype = MAX(GTYPE_MEDIUM, dirship->primtype);
      else if (argv[3] == "heavy")
        ship.primtype = MAX(GTYPE_HEAVY, dirship->primtype);
      else {
        g.out << "No such caliber.\n";
        return;
      }
      ship.primtype =
          MIN(Shipdata[dirship->build_type][ABIL_PRIMARY], ship.primtype);
    } else {
      g.out << "No such gun characteristic.\n";
      return;
    }
  } else if (argv[1] == "secondary" &&
             Shipdata[dirship->build_type][ABIL_SECONDARY]) {
    if (argv[2] == "strength") {
      if (ship.sectype == GTYPE_NONE) {
        g.out << "No caliber defined.\n";
        return;
      }
      ship.secondary = std::stoi(argv[3]);
      ship.secondary = MAX(ship.secondary, dirship->secondary);
    } else if (argv[2] == "caliber") {
      if (argv[3] == "light")
        ship.sectype = MAX(GTYPE_LIGHT, dirship->sectype);
      else if (argv[3] == "medium")
        ship.sectype = MAX(GTYPE_MEDIUM, dirship->sectype);
      else if (argv[3] == "heavy")
        ship.sectype = MAX(GTYPE_HEAVY, dirship->sectype);
      else {
        g.out << "No such caliber.\n";
        return;
      }
      ship.sectype =
          MIN(Shipdata[dirship->build_type][ABIL_SECONDARY], ship.sectype);
    } else {
      g.out << "No such gun characteristic.\n";
      return;
    }
  } else if (argv[1] == "cew" && Shipdata[dirship->build_type][ABIL_CEW]) {
    if (!Cew(race)) {
      g.out << "Your race cannot build confined energy weapons.\n";
      return;
    }
    if (!Shipdata[dirship->build_type][ABIL_CEW]) {
      g.out << "This kind of ship cannot mount confined energy weapons.\n";
      return;
    }
    value = std::stoi(argv[3]);
    if (argv[2] == "strength") {
      ship.cew = value;
    } else if (argv[2] == "range") {
      ship.cew_range = value;
    } else {
      g.out << "No such option for CEWs.\n";
      return;
    }
  } else if (argv[1] == "laser" && Shipdata[dirship->build_type][ABIL_LASER]) {
    if (!Laser(race)) {
      g.out << "Your race cannot build lasers.\n";
      return;
    }
    if (Shipdata[dirship->build_type][ABIL_LASER])
      ship.laser = 1;
    else {
      g.out << "That ship cannot be fitted with combat lasers.\n";
      return;
    }
  } else {
    g.out << "That characteristic either doesn't exist or can't be modified.\n";
    return;
  }

  /* check to see whether this ship can actually be built by this player */
  if ((complex = complexity(ship)) > race.tech) {
    sprintf(buf, "This upgrade requires an engineering technology of %.1f.\n",
            complex);
    notify(Playernum, Governor, buf);
    return;
  }

  /* check to see if the new ship will actually fit inside the hanger if it is
     on another ship. Maarten */
  std::optional<Ship> s2;
  if (dirship->whatorbits == ScopeLevel::LEVEL_SHIP) {
    s2 = getship(dirship->destshipno);
    if (s2->max_hanger - (s2->hanger - dirship->size) < ship_size(ship)) {
      sprintf(buf, "Not enough free hanger space on %c%ld.\n",
              Shipltrs[s2->type], dirship->destshipno);
      notify(Playernum, Governor, buf);
      sprintf(
          buf, "%d more needed.\n",
          ship_size(ship) - (s2->max_hanger - (s2->hanger - dirship->size)));
      notify(Playernum, Governor, buf);
      return;
    }
  }

  /* compute new ship costs and see if the player can afford it */
  newcost = race.God ? 0 : (int)cost(ship);
  oldcost = race.God ? 0 : dirship->build_cost;
  netcost = race.God ? 0 : 2 * (newcost - oldcost); /* upgrade is expensive */
  if (newcost < oldcost) {
    g.out << "You cannot downgrade ships!\n";
    return;
  }
  if (!race.God) netcost += !netcost;

  if (netcost > dirship->resource) {
    sprintf(buf, "Old value %dr   New value %dr\n", oldcost, newcost);
    notify(Playernum, Governor, buf);
    sprintf(buf, "You need %d resources on board to make this modification.\n",
            netcost);
    notify(Playernum, Governor, buf);
  } else if (netcost || race.God) {
    sprintf(buf, "Old value %dr   New value %dr\n", oldcost, newcost);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Characteristic modified at a cost of %d resources.\n",
            netcost);
    notify(Playernum, Governor, buf);
    bcopy(&ship, &*dirship, sizeof(Ship));
    dirship->resource -= netcost;
    if (dirship->whatorbits == ScopeLevel::LEVEL_SHIP) {
      s2->hanger -= dirship->size;
      dirship->size = ship_size(*dirship);
      s2->hanger += dirship->size;
      putship(&*s2);
    }
    dirship->size = ship_size(*dirship);
    dirship->base_mass = getmass(*dirship);
    dirship->build_cost = race.God ? 0 : cost(*dirship);
    dirship->complexity = complexity(*dirship);

    putship(&*dirship);
  } else
    g.out << "You can not make this modification.\n";
}
}  // namespace GB::commands