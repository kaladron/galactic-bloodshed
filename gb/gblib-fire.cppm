// SPDX-License-Identifier: Apache-2.0

export module gblib:fire;

import :planet;
import :ships;
import :types;

export int retal_strength(const Ship &);
export int adjacent(int, int, int, int, const Planet &);
export int check_retal_strength(const Ship &ship);
export bool has_planet_defense(const shipnum_t shipno,
                               const player_t Playernum);
export void check_overload(Ship &ship, int cew, int *strength);
