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

/// Player membership, invitation, and pledge summary for `block player <p>`.
struct BlockPlayerReport {
  player_t player{0};
  std::string race_name{};
  std::vector<player_t> member_blocks{};
  std::vector<player_t> invited_blocks{};
  std::vector<player_t> pledged_blocks{};
};

/// Single member race row in a specific alliance block report (`block <p>`).
struct BlockMemberRow {
  player_t player{0};
  std::string name{};
  std::string troops{};
  std::string pop{};
  std::string money{};
  std::string ships{};
  std::string planets{};
  std::string resource{};
  std::string fuel{};
  std::string destruct{};
  Percentage know{0};
};

/// Specific alliance block power report (`block <p>`).
struct BlockDetailReport {
  std::string block_name{};
  std::string motto{};
  std::vector<BlockMemberRow> members{};
};

/// Single alliance block summary row in a global `block` report.
struct BlockSummaryRow {
  player_t player{0};
  std::string name{};
  std::uint32_t members{0};
  std::string money{};
  std::string popn{};
  std::string ships{};
  std::string systems{};
  std::string resource{};
  std::string fuel{};
  std::string destruct{};
  std::string vps{};
  Percentage know{0};
};

/// Global alliance block power report (`block`).
struct BlockListReport {
  std::string update_time{};
  std::vector<BlockSummaryRow> blocks{};
};

/// View model for `block` command reports.
struct BlockViewModel {
  std::optional<BlockPlayerReport> player_report{std::nullopt};
  std::optional<BlockDetailReport> detail_report{std::nullopt};
  std::optional<BlockListReport> list_report{std::nullopt};
};

/// Renders a `BlockViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_block(const BlockViewModel& vm);

/// Serializes a `BlockViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"block","data":{...}}\n`).
[[nodiscard]] std::string render_json_block(const BlockViewModel& vm);

/// Dispatches `BlockViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const BlockViewModel& vm);

/// Single colonized planet row in a `colonies` report.
struct ColonyRow {
  char type_symbol{' '};
  std::string star_name{};
  std::string planet_name{};
  bool autorep{false};
  governor_t governor{0};
  std::uint32_t sectors_owned{0};
  money_t tech_invest{0};
  population_t popn{0};
  resource_t crystals{0};
  resource_t resource{0};
  resource_t destruct{0};
  resource_t fuel{0};
  Percentage tax{0};
  Percentage newtax{0};
  double compatibility{0.0};
  Percentage toxicity{0};
  Percentage comread{0};
  Percentage mob_set{0};
  std::vector<player_t> alien_owners{};
};

/// View model for `colonies` command reports.
struct ColoniesViewModel {
  std::vector<std::string> errors{};
  std::vector<ColonyRow> rows{};
};

/// Renders a `ColoniesViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_colonies(const ColoniesViewModel& vm);

/// Serializes a `ColoniesViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"colonies","data":{...}}\n`).
[[nodiscard]] std::string render_json_colonies(const ColoniesViewModel& vm);

/// Dispatches `ColoniesViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ColoniesViewModel& vm);

/// Single player row in a `power` report.
struct PowerRow {
  int rank{0};
  player_t player{0};
  std::string alliance{};
  std::string name{};
  std::string vp{};
  std::string mil{};
  std::string civ{};
  std::string cash{};
  std::string ships{};
  std::string planets{};
  std::string resource{};
  std::string fuel{};
  std::string destruct{};
  std::string morale{};
  std::string know_or_vns{};
};

/// View model for `power` command reports.
struct PowerViewModel {
  bool show_rank_header{true};
  bool is_god{false};
  std::vector<PowerRow> rows{};
};

/// Renders a `PowerViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_power(const PowerViewModel& vm);

/// Serializes a `PowerViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"power","data":{...}}\n`).
[[nodiscard]] std::string render_json_power(const PowerViewModel& vm);

/// Dispatches `PowerViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PowerViewModel& vm);

/// Single colony technology row in a `status` (`tech_status`) report.
struct TechStatusRow {
  std::string location{};
  population_t popn{0};
  money_t tech_invest{0};
  double gain{0.0};
  double max_gain{0.0};
};

/// View model for `status` (`tech_status`) command reports.
struct TechStatusViewModel {
  std::vector<std::string> errors{};
  std::vector<TechStatusRow> rows{};
  std::optional<population_t> total_popn{std::nullopt};
  money_t total_invest{0};
  double total_gain{0.0};
  double total_max_gain{0.0};
};

/// Renders a `TechStatusViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_tech_status(const TechStatusViewModel& vm);

/// Serializes a `TechStatusViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"tech_status","data":{...}}\n`).
[[nodiscard]] std::string
render_json_tech_status(const TechStatusViewModel& vm);

/// Dispatches `TechStatusViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TechStatusViewModel& vm);

/// View model for `examine` command output.
struct ExamineViewModel {
  shipnum_t shipno{0};
  ShipType ship_type{ShipType::STYPE_POD};
  std::string description{};
  bool has_switch{false};
  bool irradiated{false};
};

/// Renders an `ExamineViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_examine(const ExamineViewModel& vm);

/// Serializes an `ExamineViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"examine","data":{...}}\n`).
[[nodiscard]] std::string render_json_examine(const ExamineViewModel& vm);

/// Dispatches `ExamineViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ExamineViewModel& vm);

/// View model for `dump` command output.
struct DumpViewModel {
  player_t recipient{0};
  std::vector<std::string> errors{};
};

/// Renders a `DumpViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_dump(const DumpViewModel& vm);

/// Serializes a `DumpViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"dump","data":{...}}\n`).
[[nodiscard]] std::string render_json_dump(const DumpViewModel& vm);

/// Dispatches `DumpViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const DumpViewModel& vm);

}  // namespace GB::presentation
