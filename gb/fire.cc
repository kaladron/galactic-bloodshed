// SPDX-License-Identifier: Apache-2.0

/// \file fire.c
/// \brief Fire at ship or planet from ship or planet

module;

import std;

module gblib;

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

/**
 * @brief Checks for overload conditions on a ship's crystal and handles the
 * consequences.
 *
 * @param ship The ship object to check for overload conditions.
 * @param cew Strength of Confined Energy Weapons.
 * @param strength A pointer to the strength value of the ship.
 */
void check_overload(Ship &ship, int cew, int *strength) {
  if (!(ship.laser && ship.fire_laser) && (cew == 0)) {
    return;
  }

  // Check to see if the ship blows up
  if (int_rand(0, *strength) >
      (int)((1.0 - .01 * ship.damage) * ship.tech / 2.0)) {
    std::string message = std::format(
        "{}: Matter-antimatter EXPLOSION from overloaded crystal on {}\n",
        dispshiploc(ship), ship_to_string(ship));
    kill_ship(ship.owner, &ship);
    *strength = 0;
    warn(ship.owner, ship.governor, message);
    post(message, NewsType::COMBAT);
    notify_star(ship.owner, ship.governor, ship.storbits, message);
  } else if (int_rand(0, *strength) >
             (int)((1.0 - .01 * ship.damage) * ship.tech / 4.0)) {
    std::string message =
        std::format("{}: Crystal damaged from overloading on {}.\n",
                    dispshiploc(ship), ship_to_string(ship));
    ship.fire_laser = 0;
    ship.mounted = 0;
    *strength = 0;
    warn(ship.owner, ship.governor, message);
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
  if (std::abs(fy - ty) <= 1) {
    if (std::abs(fx - tx) <= 1) return 1;
    if (fx == p.Maxx - 1 && tx == 0) return 1;
    if (fx == 0 && tx == p.Maxx - 1) return 1;

    return 0;
  }
  return 0;
}
