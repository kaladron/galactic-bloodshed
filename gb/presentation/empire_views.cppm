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

/// View model for `profile` command when inspecting one's own race.
struct ProfileSelfViewModel {
  std::string race_name{};
  player_t player{0};
  bool is_god{false};
  std::string personal_info{};
  std::string home_star_name{};
  std::string home_planet_name{};
  std::optional<shipnum_t> gov_ship{std::nullopt};
  long morale{0};
  turn_t updates_active{0};
  double gun_range{0.0};
  double space_tele_range{0.0};
  double ground_tele_range{0.0};
  bool is_metamorph{false};
  fertilize_t fertilize{0};
  birthrate_t birthrate{0.0};
  mass_t mass{0.0};
  fighters_t fighters{0};
  metabolism_t metabolism{0.0};
  sexes_t number_sexes{1};
  adventurism_t adventurism{0.0};
  iq_t iq{0};
  double tech{0.0};
  temperature_t temp{0};
  ConditionValues conditions{};
  SectorCompatibilities sector_compat{};
  TechDiscoveries discoveries{};
};

/// Renders a `ProfileSelfViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_profile_self(const ProfileSelfViewModel& vm);

/// Serializes a `ProfileSelfViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"profile_self","data":{...}}\n`).
[[nodiscard]] std::string
render_json_profile_self(const ProfileSelfViewModel& vm);

/// Dispatches `ProfileSelfViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ProfileSelfViewModel& vm);

/// View model for `profile <race>` command when inspecting another race.
struct ProfileOtherViewModel {
  std::string race_name{};
  player_t player{0};
  bool show_deity_status{false};
  std::string personal_info{};
  Percentage know{0};
  std::optional<bool> is_metamorph{std::nullopt};
  std::string fertilize{};
  std::string temp{};
  std::string birthrate{};
  std::string methane{};
  std::string mass{};
  std::string oxygen{};
  std::string gun_range{};
  std::string fighters{};
  std::string helium{};
  std::string space_tele_range{};
  std::string metabolism{};
  std::string nitrogen{};
  std::string ground_tele_range{};
  std::string number_sexes{};
  std::string co2{};
  std::string adventurism{};
  std::string hydrogen{};
  std::string iq{};
  std::string sulfur{};
  std::string tech{};
  std::string other{};
  std::string morale{};
  std::string best_sector{};
};

/// Renders a `ProfileOtherViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_profile_other(const ProfileOtherViewModel& vm);

/// Serializes a `ProfileOtherViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"profile_other","data":{...}}\n`).
[[nodiscard]] std::string
render_json_profile_other(const ProfileOtherViewModel& vm);

/// Dispatches `ProfileOtherViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ProfileOtherViewModel& vm);

}  // namespace GB::presentation
