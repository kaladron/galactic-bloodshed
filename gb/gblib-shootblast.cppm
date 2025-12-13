// SPDX-License-Identifier: Apache-2.0

export module gblib:shootblast;

import :misc;
import :race;
import :ships;

// Damage, Short, Long
export std::optional<std::tuple<int, std::string, std::string>>
shoot_ship_to_ship(EntityManager& em, const Ship& attacker, Ship& target,
                   int cew_strength, int range, bool ignore = false);
export int shoot_planet_to_ship(EntityManager& em, Race& race, Ship& target,
                                int strength, char* long_msg, char* short_msg);
export int shoot_ship_to_planet(EntityManager& em, const Ship& attacker,
                                Planet& target, int strength, int range,
                                int accuracy, SectorMap& sector_map,
                                int sector_x, int sector_y, char* long_msg,
                                char* short_msg);
export std::pair<int, int> hit_odds(double range, double tech, int fdam,
                                    int fev, int tev, int fspeed, int tspeed,
                                    int body, guntype_t caliber, int defense);
export double tele_range(ShipType tech_level, double base_range);
export guntype_t current_caliber(const Ship& ship);
export std::tuple<int, int, int, int> do_collateral(Ship& ship, int damage);
export int planet_guns(long planet_id);

/**
 * @brief Calculates the gun range for a given race based on its technology
 * level.
 *
 * @param r The race whose gun range is to be calculated.
 * @return The computed gun range as a double.
 */
export constexpr double gun_range(const Race& r) {
  return logscale((int)(r.tech + 1.0)) * SYSTEMSIZE;
}

/**
 * @brief Calculates the gun range for a given ship based on its technology
 * level.
 *
 * @param s The ship whose gun range is to be calculated.
 * @return The computed gun range as a double.
 */
export constexpr double gun_range(const Ship& s) {
  return logscale((int)(s.tech() + 1.0)) * SYSTEMSIZE;
}
