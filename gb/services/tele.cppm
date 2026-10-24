// SPDX-License-Identifier: Apache-2.0

/// \file tele.cppm
/// \brief Telegram and news service declarations.

export module gb.services:tele;

import std;

import dallib;
import gb.entities;
import :gameobj;

export void check_for_telegrams(GameObj&);
export void purge(EntityManager&);
export void post(EntityManager&, std::string, NewsType);
export void push_telegram(EntityManager&, player_t recipient, governor_t gov,
                          std::string_view msg);
export void push_telegram_race(EntityManager&, const player_t,
                               std::string_view);
export void telegram_star(EntityManager&, starnum_t, player_t sender,
                          governor_t sender_gov, const std::string& message);
export [[nodiscard]] std::vector<TelegramItem>
teleg_read(EntityManager& em, player_t player, governor_t governor);
export [[nodiscard]] std::vector<NewsItem> news_read(EntityManager& em,
                                                     player_t player,
                                                     governor_t governor,
                                                     NewsType type);
