// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file fire.c
/// \brief Fire at ship or planet from ship or planet

import gblib;
import std.compat;

#include "gb/fire.h"

#include <strings.h>

#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/tele.h"
#include "gb/tweakables.h"

// check to see if there are any planetary defense networks on the planet
bool has_planet_defense(const shipnum_t shipno, const player_t Playernum) {
  Shiplist shiplist(shipno);
  for (const auto &ship : shiplist) {
    if (ship.alive && ship.type == ShipType::OTYPE_PLANDEF &&
        ship.owner != Playernum) {
      return true;
    }
  }
  return false;
}

void check_overload(Ship *ship, int cew, int *strength) {
  if ((ship->laser && ship->fire_laser) || cew) {
    if (int_rand(0, *strength) >
        (int)((1.0 - .01 * ship->damage) * ship->tech / 2.0)) {
      /* check to see if the ship blows up */
      sprintf(buf,
              "%s: Matter-antimatter EXPLOSION from overloaded crystal on %s\n",
              dispshiploc(*ship).c_str(), ship_to_string(*ship).c_str());
      kill_ship((int)(ship->owner), ship);
      *strength = 0;
      warn(ship->owner, ship->governor, buf);
      post(buf, NewsType::COMBAT);
      notify_star(ship->owner, ship->governor, ship->storbits, buf);
    } else if (int_rand(0, *strength) >
               (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0)) {
      sprintf(buf, "%s: Crystal damaged from overloading on %s.\n",
              dispshiploc(*ship).c_str(), ship_to_string(*ship).c_str());
      ship->fire_laser = 0;
      ship->mounted = 0;
      *strength = 0;
      warn(ship->owner, ship->governor, buf);
    }
  }
}

int check_retal_strength(const Ship &ship) {
  // irradiated ships dont retaliate
  if (!ship.active || !ship.alive) return 0;

  if (laser_on(ship))
    return MIN(ship.fire_laser, (int)ship.fuel / 2);
  else
    return retal_strength(ship);
}

int retal_strength(const Ship &s) {
  int strength = 0;
  int avail = 0;

  if (!s.alive) return 0;
  if (!Shipdata[s.type][ABIL_SPEED] && !landed(s)) return 0;
  /* land based ships */
  if (!s.popn && (s.type != ShipType::OTYPE_BERS)) return 0;

  if (s.guns == PRIMARY)
    avail = (s.type == ShipType::STYPE_FIGHTER ||
             s.type == ShipType::OTYPE_AFV || s.type == ShipType::OTYPE_BERS)
                ? s.primary
                : MIN(s.popn, s.primary);
  else if (s.guns == SECONDARY)
    avail = (s.type == ShipType::STYPE_FIGHTER ||
             s.type == ShipType::OTYPE_AFV || s.type == ShipType::OTYPE_BERS)
                ? s.secondary
                : MIN(s.popn, s.secondary);
  else
    avail = 0;

  avail = MIN(s.retaliate, avail);
  strength = MIN(s.destruct, avail);
  return strength;
}

int adjacent(int fx, int fy, int tx, int ty, const Planet &p) {
  if (abs(fy - ty) <= 1) {
    if (abs(fx - tx) <= 1) return 1;
    if (fx == p.Maxx - 1 && tx == 0) return 1;
    if (fx == 0 && tx == p.Maxx - 1) return 1;

    return 0;
  }
  return 0;
}
