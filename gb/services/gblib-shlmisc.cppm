// SPDX-License-Identifier: Apache-2.0

export module gblib:shlmisc;

import gb.entities;
import :gameobj;

export bool authorized(governor_t, const Ship&);
export shipnum_t start_shiplist(GameObj&, const std::string_view);
export shipnum_t do_shiplist(Ship**, shipnum_t*);
export bool in_list(const player_t, std::string_view, const Ship&, shipnum_t*);
export void notify_dont_own_ship(EntityManager&, player_t, governor_t,
                                 shipnum_t);
export void notify_dont_own_ship(const GameObj&, shipnum_t);
export std::tuple<player_t, governor_t>
getracenum(EntityManager&, const std::string&, const std::string&);
export std::optional<std::tuple<int, int, int, int>>
get4args(std::string_view s);
export player_t get_player(EntityManager&, const std::string&);
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
