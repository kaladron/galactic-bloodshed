// SPDX-License-Identifier: Apache-2.0

export module gblib:shootblast;

import :misc;
import :race;
import :ships;

export int shoot_ship_to_ship(const Ship &attacker, Ship &target, int strength,
                              int range, bool ignore, char *long_msg,
                              char *short_msg);
export int shoot_planet_to_ship(Race &race, Ship &target, int strength,
                                char *long_msg, char *short_msg);
export int shoot_ship_to_planet(Ship &attacker, Planet &target, int strength,
                                int range, int accuracy, SectorMap &sector_map,
                                int sector_x, int sector_y, char *long_msg,
                                char *short_msg);
export std::pair<int, int> hit_odds(double range, double tech, int fdam,
                                    int fev, int tev, int fspeed, int tspeed,
                                    int body, guntype_t caliber, int defense);
export double tele_range(ShipType tech_level, double base_range);
export guntype_t current_caliber(const Ship &ship);
export std::tuple<int, int, int, int> do_collateral(Ship &ship, int damage);
export int planet_guns(long planet_id);

/*
 * gun range of given ship, given race and ship
 */
export constexpr double gun_range(const Race &r) {
  return logscale((int)(r.tech + 1.0)) * SYSTEMSIZE;
}

/*
 * gun range of given ship, given race and ship
 */
export constexpr double gun_range(const Ship &s) {
  return logscale((int)(s.tech + 1.0)) * SYSTEMSIZE;
}
