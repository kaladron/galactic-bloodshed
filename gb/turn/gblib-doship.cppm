// SPDX-License-Identifier: Apache-2.0

/// \file gblib-doship.cppm
/// \brief Module interface partition for ship turn simulation processing.

export module gblib:doship;

import :gameobj;
import :ships;
import :turnstats;

export void doship(Ship&, bool update, EntityManager&, TurnStats& stats);
export void domass(Ship&, EntityManager&);
export void doown(Ship&, EntityManager&);
export void domissile(Ship&, EntityManager&);
export void domine(Ship&, int, EntityManager&);
export void doabm(Ship&, EntityManager&);
export int do_weapon_plant(Ship&, EntityManager&);
export void do_repair(Ship& ship, EntityManager& entity_manager);
export void do_habitat(Ship& ship, EntityManager& entity_manager);
export void do_pod(Ship& ship, EntityManager& entity_manager);
export void do_canister(Ship& ship, EntityManager& entity_manager,
                        TurnStats& stats);
export void do_greenhouse(Ship& ship, EntityManager& entity_manager,
                          TurnStats& stats);
export void do_mirror(Ship& ship, EntityManager& entity_manager,
                      TurnStats& stats);
export void do_meta_infect(player_t who, starnum_t star, planetnum_t pnum,
                           Planet& p, EntityManager& entity_manager);
export int infect_planet(player_t who, starnum_t star, planetnum_t pnum,
                         EntityManager& entity_manager);
export void do_ap(Ship& ship, EntityManager& entity_manager);
export void do_god(Ship& ship, EntityManager& entity_manager);

/// \brief Top two nearest star systems identified by navigation scanning.
export struct StarTargetResult {
  starnum_t closest{0};         ///< Primary nearest star system
  starnum_t second_closest{0};  ///< Secondary nearest star system
};

/// \brief Finds the closest and second-closest star systems to the given
/// coordinates, excluding the current star system.
export StarTargetResult find_closest_stars(EntityManager& em,
                                           starnum_t current_star, double xpos,
                                           double ypos);

/// \brief Assigns destination orders to an autonomous berserker ship.
export void select_berserker_destination(EntityManager& em,
                                         AutonomousShip& ship,
                                         const TurnStats& stats);

/// \brief Assigns destination orders to an autonomous Von Neumann machine.
export void select_vn_destination(EntityManager& em, AutonomousShip& ship);
