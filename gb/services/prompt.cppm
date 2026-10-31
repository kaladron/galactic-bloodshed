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

export struct SessionScopeView {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};
  std::string star{};
  std::string planet{};
  std::string ship{};
  std::string path{};
};

export struct SessionViewportView {
  UniverseCoordinates universe_center{0.0, 0.0};
  SystemCoordinates system_center{0.0, 0.0};
  double universe_zoom{0.5};
  double system_zoom{0.5};
};

export struct SessionClockView {
  std::time_t server_time{0};
  int update_interval_minutes{0};
  segments_t segments_per_update{0};
  segments_t current_segment{0};
  std::time_t next_segment_time{0};
  turn_t next_update_number{0};
  std::time_t next_update_time{0};
};

export struct SessionView {
  player_t player{0};
  governor_t governor{1};
  std::string race_name{};
  std::string governor_name{};
  bool god{false};
  ap_t ap{0};
  ap_t global_ap{0};
  SessionScopeView scope{};
  SessionViewportView viewport{};
  toggletype toggles{};
  std::optional<SessionClockView> clock{std::nullopt};
};

export [[nodiscard]] SessionView build_session_view(const GameObj& g);

export [[nodiscard]] PromptViewModel
build_ship_prompt(EntityManager& em, player_t player, shipnum_t shipno);

export [[nodiscard]] PromptViewModel build_prompt(const GameObj& g);

export std::string format_ship_prompt(EntityManager& em, player_t player,
                                      shipnum_t shipno);

export std::string do_prompt(const GameObj& g);
