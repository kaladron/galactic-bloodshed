// SPDX-License-Identifier: Apache-2.0

export module gblib:doturncmd;

import :gameobj;
import :services;
import :types;

export void do_turn(Db&, EntityManager&, int);
export void handle_victory();
