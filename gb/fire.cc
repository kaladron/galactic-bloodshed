// SPDX-License-Identifier: Apache-2.0

/// \file fire.c
/// \brief Fire at ship or planet from ship or planet

module;

import std;

module gblib;

/**
 * @brief Checks to see if there are any planetary defense networks on the
 * planet.
 *
 * @param shipno The ship number.
 * @param Playernum The player number.
 * @return True if there are planetary defense networks, false otherwise.
 */
bool has_planet_defense(const shipnum_t shipno, const player_t Playernum) {
  Shiplist shiplist(shipno);
  for (const auto& ship : shiplist) {
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
void check_overload(Ship& ship, int cew, int* strength) {
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

int check_retal_strength(const Ship& ship) {
  // irradiated ships dont retaliate
  if (!ship.active || !ship.alive) return 0;

  if (laser_on(ship)) return MIN(ship.fire_laser, (int)ship.fuel / 2);

  return retal_strength(ship);
}

int retal_strength(const Ship& s) {
  if (!s.alive) return 0;
  if (!Shipdata[s.type][ABIL_SPEED] && !landed(s)) return 0;
  /* land based ships */
  if (!s.popn && (s.type != ShipType::OTYPE_BERS)) return 0;

  auto avail = [&]() {
    if (s.guns == PRIMARY)
      return (s.type == ShipType::STYPE_FIGHTER ||
              s.type == ShipType::OTYPE_AFV || s.type == ShipType::OTYPE_BERS)
                 ? s.primary
                 : MIN(s.popn, s.primary);
    if (s.guns == SECONDARY)
      return (s.type == ShipType::STYPE_FIGHTER ||
              s.type == ShipType::OTYPE_AFV || s.type == ShipType::OTYPE_BERS)
                 ? s.secondary
                 : MIN(s.popn, s.secondary);

    return 0UL;
  }();

  avail = MIN(s.retaliate, avail);
  int strength = MIN(s.destruct, avail);
  return strength;
}
