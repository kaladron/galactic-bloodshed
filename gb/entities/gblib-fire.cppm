// SPDX-License-Identifier: Apache-2.0

export module gblib:fire;

import :gameobj;
import :planet;
import :ships;
import :types;

export bool has_planet_defense(EntityManager&, starnum_t, planetnum_t,
                               player_t);
export void check_overload(EntityManager& entity_manager, Ship& ship, int cew,
                           weapon_power_t* strength);
