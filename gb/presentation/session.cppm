// SPDX-License-Identifier: Apache-2.0

/// \file session.cppm
/// \brief Session state view models and canonical player prompt rendering.

export module gb.presentation.session;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import std;

export namespace GB::presentation {

struct PromptViewModel {
  ap_t ap{0};
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  std::string star{};
  std::string planet{};
  std::string ship{};
  std::string path{};
};

struct SessionScopeView {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};
  std::string star{};
  std::string planet{};
  std::string ship{};
  std::string path{};
};

struct SessionViewportView {
  UniverseCoordinates universe_center{0.0, 0.0};
  SystemCoordinates system_center{0.0, 0.0};
  double universe_zoom{0.5};
  double system_zoom{0.5};
};

struct SessionClockView {
  std::time_t server_time{0};
  std::time_t server_start_time{0};
  std::time_t last_update_time{0};
  std::time_t last_segment_time{0};
  int update_interval_minutes{0};
  segments_t segments_per_update{0};
  segments_t segments_done{0};
  segments_t current_segment{0};
  std::time_t next_segment_time{0};
  turn_t updates_done{0};
  turn_t next_update_number{0};
  std::time_t next_update_time{0};
};

struct SessionView {
  player_t player{0};
  governor_t governor{1};
  std::string race_name{};
  std::string governor_name{};
  bool god{false};
  bool color{false};
  ap_t ap{0};
  ap_t global_ap{0};
  SessionScopeView scope{};
  SessionViewportView viewport{};
  toggletype toggles{};
  std::optional<SessionClockView> clock{std::nullopt};
};

[[nodiscard]] SessionView build_session_view(const GameObj& g);

[[nodiscard]] PromptViewModel
build_ship_prompt(EntityManager& em, player_t player, shipnum_t shipno);

[[nodiscard]] PromptViewModel build_prompt(const GameObj& g);

[[nodiscard]] std::string format_ship_prompt(EntityManager& em, player_t player,
                                             shipnum_t shipno);

/// Canonical prompt and session-state emitter appended once per command by
/// Server::process_command(). Intentionally NOT exposed as a
/// render_for_mode(UiMode, const SessionView&) overload so commands cannot
/// emit duplicate {"type":"session"} stanzas via g.present().
[[nodiscard]] std::string do_prompt(const GameObj& g);

}  // namespace GB::presentation
