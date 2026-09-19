// SPDX-License-Identifier: Apache-2.0

export module gblib:shlmisc;

import gb.entities;
import gb.services;
import std;

export bool authorized(governor_t, const Ship&);
export shipnum_t start_shiplist(GameObj&, const std::string_view);
export shipnum_t do_shiplist(Ship**, shipnum_t*);
export bool in_list(const player_t, std::string_view, const Ship&, shipnum_t*);
export std::optional<std::tuple<int, int, int, int>>
get4args(std::string_view s);
export armor_t getdefense(EntityManager&, const Ship&);
export void capture_stuff(const Ship&, GameObj&);
export std::string prin_ship_orbits(EntityManager&, const Ship&);
export std::string format_ship_dest(EntityManager&, const Ship&);
export void moveship(EntityManager&, Ship& ship, bool is_update,
                     bool send_messages, bool checking_fuel);
export void msg_OOF(EntityManager&, const Ship& ship);
export bool followable(EntityManager&, const Ship& ship, const Ship& target);
export std::string dispshiploc_brief(EntityManager&, const Ship&);
export std::string dispshiploc(EntityManager&, const Ship&);
