// SPDX-License-Identifier: Apache-2.0

export module gblib:doship;

import :gameobj;
import :ships;

export void doship(Ship&, int, EntityManager&);
export void domass(Ship&);
export void doown(Ship&);
export void domissile(Ship&, EntityManager&);
export void domine(Ship&, int, EntityManager&);
export void doabm(Ship&, EntityManager&);
export int do_weapon_plant(Ship&);
