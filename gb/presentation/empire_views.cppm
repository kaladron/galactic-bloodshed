// SPDX-License-Identifier: Apache-2.0

/// \file empire_views.cppm
/// \brief Empire, diplomatic, and player status view models and renderers.

export module gb.presentation.empire_views;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import std;

export namespace GB::presentation {

/// Single row in a `relation` racial relations report.
struct RelationRow {
  player_t player{0};
  bool is_known_morph{false};
  Percentage know{0};
  std::string race_name{};
  std::string yours{};
  std::string theirs{};
};

/// View model for `relation` diplomatic standing reports.
struct RelationViewModel {
  player_t subject_player{0};
  std::string subject_race_name{};
  std::vector<RelationRow> rows{};
};

/// Renders a `RelationViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_relation(const RelationViewModel& vm);

/// Serializes a `RelationViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"relation","data":{...}}\n`).
[[nodiscard]] std::string render_json_relation(const RelationViewModel& vm);

/// Dispatches `RelationViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const RelationViewModel& vm);

/// Single connected player row in a `who` report.
struct WhoRow {
  std::string race_name{};
  std::string governor_name{};
  player_t player{0};
  governor_t governor{0};
  std::time_t idle_seconds{0};
  std::string star_name{};
  bool gag{false};
  bool invisible{false};
};

/// View model for `who` command active session listings.
struct WhoViewModel {
  std::time_t now{0};
  std::vector<WhoRow> rows{};
  bool show_cowards{SHOW_COWARDS};
  int coward_count{0};
};

/// Renders a `WhoViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_who(const WhoViewModel& vm);

/// Serializes a `WhoViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"who","data":{...}}\n`).
[[nodiscard]] std::string render_json_who(const WhoViewModel& vm);

/// Dispatches `WhoViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const WhoViewModel& vm);

/// Single result entry in a `whois` / `identify` query.
struct WhoisEntry {
  player_t player{0};
  std::optional<governor_t> governor{std::nullopt};
  std::string race_name{};
  std::optional<std::string> governor_name{std::nullopt};
  std::optional<std::string> error{std::nullopt};
};

/// View model for `whois` / `identify` command results.
struct WhoisViewModel {
  std::vector<WhoisEntry> entries{};
};

/// Renders a `WhoisViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_whois(const WhoisViewModel& vm);

/// Serializes a `WhoisViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"whois","data":{...}}\n`).
[[nodiscard]] std::string render_json_whois(const WhoisViewModel& vm);

/// Dispatches `WhoisViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const WhoisViewModel& vm);

}  // namespace GB::presentation
