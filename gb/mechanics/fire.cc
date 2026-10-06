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
    if (s.type() == ShipType::OTYPE_PLANDEF && s.owner() != Playernum) {
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
 * @return Structured ReactorOverloadEvent if crystal burnout or explosion
 * occurs, std::nullopt otherwise.
 */
std::optional<ReactorOverloadEvent>
check_overload(EntityManager& entity_manager, Ship& ship, int cew,
               weapon_power_t* strength) {
  if (!(ship.laser() && ship.fire_laser()) && (cew == 0)) {
    return std::nullopt;
  }

  // Check to see if the ship blows up
  if (int_rand(0, *strength) >
      static_cast<int>((1.0 - .01 * ship.damage()) * ship.tech() / 2.0)) {
    ReactorOverloadEvent event{
        .outcome = ReactorOverloadOutcome::ShipExploded,
        .owner = ship.owner(),
        .governor = ship.governor(),
        .scope = ship.whatorbits(),
        .star_id = ship.storbits(),
        .location_display = dispshiploc(entity_manager, ship),
        .ship_display = std::format("{}", ship),
    };
    entity_manager.kill_ship(ship.owner(), ship);
    *strength = 0;
    return event;
  }
  if (int_rand(0, *strength) >
      static_cast<int>((1.0 - .01 * ship.damage()) * ship.tech() / 4.0)) {
    ship.fire_laser() = 0;
    ship.mounted() = 0;
    *strength = 0;
    return ReactorOverloadEvent{
        .outcome = ReactorOverloadOutcome::CrystalDamaged,
        .owner = ship.owner(),
        .governor = ship.governor(),
        .scope = ship.whatorbits(),
        .star_id = ship.storbits(),
        .location_display = dispshiploc(entity_manager, ship),
        .ship_display = std::format("{}", ship),
    };
  }
  return std::nullopt;
}
