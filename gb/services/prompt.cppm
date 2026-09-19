// SPDX-License-Identifier: Apache-2.0

/// \file prompt.cppm
/// \brief Module partition for player command prompt generation.

module;

import std;

export module gb.services:prompt;

import gb.entities;
import :gameobj;
import :services;

export std::string format_ship_prompt(EntityManager& em, player_t player,
                                      shipnum_t shipno);

export std::string do_prompt(const GameObj& g);
