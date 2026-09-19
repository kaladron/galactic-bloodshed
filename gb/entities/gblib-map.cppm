// SPDX-License-Identifier: Apache-2.0

export module gblib:map;

import gb.entities;
import gb.services;
import std;

export char desshow(player_t, governor_t, const Race&, const Sector&);
export void show_map(GameObj& g, starnum_t, planetnum_t, const Planet&);
