// SPDX-License-Identifier: Apache-2.0

export module gblib:doturncmd;

import :gameobj;
import :services;
import :types;

export void do_turn(EntityManager&, void*,
                    int);  // void* is actually SessionRegistry*
export void handle_victory(EntityManager&);
