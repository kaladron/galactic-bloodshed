// SPDX-License-Identifier: Apache-2.0

export module gblib:doship;

import :gameobj;
import :ships;
import :turnstats;

export void doship(Ship&, int, EntityManager&, TurnStats& stats);
export void domass(Ship&, EntityManager&);
export void doown(Ship&, EntityManager&);
export void domissile(Ship&, EntityManager&);
export void domine(Ship&, int, EntityManager&);
export void doabm(Ship&, EntityManager&);
export int do_weapon_plant(Ship&, EntityManager&);
