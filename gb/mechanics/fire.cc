// SPDX-License-Identifier: Apache-2.0

/// \file fire.cc
/// \brief Fire at ship or planet from ship or planet

module;

import std;

module gb.mechanics;

/**
 * @brief Checks to see if there are any planetary defense networks on the
 * planet.
 *
 * @param star_id The star id.
 * @param planet_order The planet order.
 * @param Playernum The player number.
 * @return True if there are planetary defense networks, false otherwise.
 */
bool has_planet_defense(EntityManager& entity_manager, const starnum_t star_id,
                        const planetnum_t planet_order,
                        const player_t Playernum) {
  for (const Ship& s :
       ShipList::readonly_on_planet(entity_manager, star_id, planet_order)) {
    if (s.alive() && s.type() == ShipType::OTYPE_PLANDEF &&
        s.owner() != Playernum) {
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
void check_overload(EntityManager& entity_manager, Ship& ship, int cew,
                    weapon_power_t* strength) {
  if (!(ship.laser() && ship.fire_laser()) && (cew == 0)) {
    return;
  }

  // Check to see if the ship blows up
  if (int_rand(0, *strength) >
      (int)((1.0 - .01 * ship.damage()) * ship.tech() / 2.0)) {
    std::string message = std::format(
        "{}: Matter-antimatter EXPLOSION from overloaded crystal on {}\n",
        dispshiploc(entity_manager, ship), ship);
    entity_manager.kill_ship(ship.owner(), ship);
    *strength = 0;
    push_telegram(entity_manager, ship.owner(), ship.governor(), message);
    post(entity_manager, message, NewsType::COMBAT);
    telegram_star(entity_manager, ship.storbits(), ship.owner(),
                  ship.governor(), message);
  } else if (int_rand(0, *strength) >
             (int)((1.0 - .01 * ship.damage()) * ship.tech() / 4.0)) {
    std::string message =
        std::format("{}: Crystal damaged from overloading on {}.\n",
                    dispshiploc(entity_manager, ship), ship);
    ship.fire_laser() = 0;
    ship.mounted() = 0;
    *strength = 0;
    push_telegram(entity_manager, ship.owner(), ship.governor(), message);
  }
}
