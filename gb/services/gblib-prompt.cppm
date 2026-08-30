// SPDX-License-Identifier: Apache-2.0

/// \file gblib-prompt.cppm
/// \brief Module partition for player command prompt generation.

module;

import std;

export module gblib:prompt;

import :gameobj;
import :services;
import :types;

export std::string format_ship_prompt(EntityManager& em, player_t player,
                                      shipnum_t shipno);

export std::string do_prompt(const GameObj& g);
