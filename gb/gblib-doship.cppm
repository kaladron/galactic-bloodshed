// SPDX-License-Identifier: Apache-2.0

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
