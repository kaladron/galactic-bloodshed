// SPDX-License-Identifier: Apache-2.0

export module gblib:tele;

import std;

import :types;

export void check_for_telegrams(GameObj &);
export void purge();
export void post(std::string, NewsType);
export void push_telegram_race(const player_t, const std::string &);
export void teleg_read(GameObj &);
export void news_read(NewsType type, GameObj &g);
export void push_telegram(const player_t recipient, const governor_t gov,
                          std::string_view msg);
