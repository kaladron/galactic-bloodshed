// SPDX-License-Identifier: Apache-2.0

export module gblib:shootblast;

import :misc;
import :race;
import :ships;

export int shoot_ship_to_ship(Ship &attacker, Ship &target, int strength,
                              int range, bool ignore, char *long_msg,
                              char *short_msg);
export int shoot_planet_to_ship(Race &race, Ship *target, int strength,
                                char *long_msg, char *short_msg);
export int shoot_ship_to_planet(Ship *attacker, Planet &target, int strength,
                                int range, int accuracy, SectorMap &sector_map,
                                int sector_x, int sector_y, char *long_msg,
                                char *short_msg);
export int hit_odds(double attacker_strength, int *attacker_guns,
                    double target_strength, int target_shields,
                    int target_defense, int target_damage, int target_crew,
                    int target_hull, int target_engine, int target_fuel,
                    int target_mass);
export double tele_range(int tech_level, double base_range);
export int current_caliber(const Ship &ship);
export void do_collateral(Ship *ship, int damage, int *crew_killed,
                          int *hull_damaged, int *engine_damaged,
                          int *fuel_lost);
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
