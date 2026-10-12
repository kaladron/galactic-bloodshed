// SPDX-License-Identifier: Apache-2.0

/// \file prompt.cppm
/// \brief Module partition for player command prompt generation.

module;

import std;

export module gb.services:prompt;

import gb.entities;
import :gameobj;
import :services;

export struct PromptViewModel {
  ap_t ap{0};
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  std::string star{};
  std::string planet{};
  std::string ship{};
  std::string path{};
};

export [[nodiscard]] PromptViewModel
build_ship_prompt(EntityManager& em, player_t player, shipnum_t shipno);

export [[nodiscard]] PromptViewModel build_prompt(const GameObj& g);

export std::string format_ship_prompt(EntityManager& em, player_t player,
                                      shipnum_t shipno);

export std::string do_prompt(const GameObj& g);
