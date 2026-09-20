// SPDX-License-Identifier: Apache-2.0

export module gblib:shlmisc;

import gb.entities;
import gb.services;
import std;

export shipnum_t start_shiplist(GameObj&, const std::string_view);
export shipnum_t do_shiplist(Ship**, shipnum_t*);
export bool in_list(const player_t, std::string_view, const Ship&, shipnum_t*);
export std::optional<std::tuple<int, int, int, int>>
get4args(std::string_view s);
