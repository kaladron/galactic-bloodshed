// SPDX-License-Identifier: Apache-2.0

export module gblib:shlmisc;

import :gameobj;
import :ships;
import :star;

export bool authorized(governor_t, const Ship&);
export shipnum_t start_shiplist(GameObj&, const std::string_view);
export shipnum_t do_shiplist(Ship**, shipnum_t*);
export bool in_list(const player_t, std::string_view, const Ship&, shipnum_t*);
export void DontOwnErr(int, int, shipnum_t);
export bool enufAP(player_t, governor_t, ap_t have, ap_t needed);
export std::tuple<player_t, governor_t> getracenum(const std::string&,
                                                   const std::string&);
export std::tuple<player_t, governor_t>
getracenum(EntityManager&, const std::string&, const std::string&);
export void get4args(const char* s, int* xl, int* xh, int* yl, int* yh);
export player_t get_player(const std::string&);
export void allocateAPs(const command_t&, GameObj&);
export void deductAPs(const GameObj&, ap_t, ScopeLevel);
export void deductAPs(const GameObj&, ap_t, starnum_t);
