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

}  // namespace GB::presentation
