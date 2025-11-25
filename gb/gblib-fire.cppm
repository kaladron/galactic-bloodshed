// SPDX-License-Identifier: Apache-2.0

export module gblib:fire;

import :gameobj;
import :planet;
import :ships;
import :types;

export int retal_strength(const Ship&);
export int check_retal_strength(const Ship& ship);
export bool has_planet_defense(EntityManager&, shipnum_t, player_t);
export void check_overload(Ship& ship, int cew, int* strength);
