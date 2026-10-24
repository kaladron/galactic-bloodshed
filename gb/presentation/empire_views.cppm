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
  sector_count_t sectors_owned{0};
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

/// View model for `time` command output.
struct TimeViewModel {
  std::string start_line{};
  std::string update_line{};
  std::string segment_line{};
  std::string current_time{};
};

/// Renders a `TimeViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_time(const TimeViewModel& vm);

/// Serializes a `TimeViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"time","data":{...}}\n`).
[[nodiscard]] std::string render_json_time(const TimeViewModel& vm);

/// Dispatches `TimeViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const TimeViewModel& vm);

/// View model for `schedule` command output.
struct ScheduleViewModel {
  int update_time_minutes{0};
  segments_t segments{0};
  std::string current_time{};
  segments_t next_segment_number{0};
  std::string next_segment_time{};
  turn_t next_update_number{0};
  std::string next_update_time{};
};

/// Renders a `ScheduleViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_schedule(const ScheduleViewModel& vm);

/// Serializes a `ScheduleViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"schedule","data":{...}}\n`).
[[nodiscard]] std::string render_json_schedule(const ScheduleViewModel& vm);

/// Dispatches `ScheduleViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ScheduleViewModel& vm);

/// Single player ranking row in a `victory` report.
struct VictoryRow {
  int rank{0};
  bool is_metamorph{false};
  player_t player{0};
  std::string name{};
  victory_score_t raw_score{0};
  double tech{0.0};
  iq_t iq{0};
  std::string password{};
  std::string gov_password{};
};

/// View model for `victory` command output.
struct VictoryViewModel {
  bool is_god{false};
  std::vector<VictoryRow> rows{};
};

/// Renders a `VictoryViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_victory(const VictoryViewModel& vm);

/// Serializes a `VictoryViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"victory","data":{...}}\n`).
[[nodiscard]] std::string render_json_victory(const VictoryViewModel& vm);

/// Dispatches `VictoryViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const VictoryViewModel& vm);

using ::PromptViewModel;

/// Renders a `PromptViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_prompt(const PromptViewModel& vm);

/// Serializes a `PromptViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"prompt","data":{...}}\n`).
[[nodiscard]] std::string render_json_prompt(const PromptViewModel& vm);

/// Dispatches `PromptViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PromptViewModel& vm);

/// View model for plain-text output lines wrapped in `UiMode::JSON`.
struct TextViewModel {
  std::string message{};
};

/// Renders a `TextViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_text(const TextViewModel& vm);

/// Serializes a `TextViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"text","data":{"message":"..."}}\n`).
[[nodiscard]] std::string render_json_text(const TextViewModel& vm);

/// Dispatches `TextViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const TextViewModel& vm);

/// View model for player login session details (`auth.cc`).
struct LoginViewModel {
  player_t race_id{0};
  governor_t gov_id{0};
  std::string race_name{};
  std::string gov_name{};
  bool invisible{false};
  std::string last_login{};
  std::optional<shipnum_t> gov_ship{std::nullopt};
  long morale{0};
};

/// Renders the initial login banner for `UiMode::ASCII`.
[[nodiscard]] std::string render_login_banner(const LoginViewModel& vm);

/// Renders the post-time login status (last login, gov center, morale) for
/// `UiMode::ASCII`.
[[nodiscard]] std::string render_login_status(const LoginViewModel& vm);

/// Renders the full login summary for `UiMode::ASCII`.
[[nodiscard]] std::string render_login(const LoginViewModel& vm);

/// Serializes a `LoginViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"login","data":{...}}\n`).
[[nodiscard]] std::string render_json_login(const LoginViewModel& vm);

/// Dispatches `LoginViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const LoginViewModel& vm);

/// Single market lot row in a `bid` commodities market listing.
struct CommodityLotRow {
  int lot{0};
  bool deliver{false};
  resource_t amount{0};
  CommodType type{CommodType::RESOURCE};
  player_t owner{0};
  player_t bidder{0};
  money_t bid{0};
  double cost_per_unit{0.0};
  money_t shipping_cost{0};
  std::string destination{};
};

/// View model for `bid` / `bid <type>` commodities market listing.
struct CommodityMarketViewModel {
  std::optional<CommodType> filter{std::nullopt};
  std::vector<CommodityLotRow> rows{};
};

[[nodiscard]] std::string
render_commodity_market(const CommodityMarketViewModel& vm);
[[nodiscard]] std::string
render_json_commodity_market(const CommodityMarketViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const CommodityMarketViewModel& vm);

/// Result of placing a bid on a commodity lot (`bid <lot> <bid>`).
struct BidPlacedResult {
  int lot{0};
  money_t bid{0};
  money_t shipping_cost{0};
};

[[nodiscard]] std::string render_bid_placed_result(const BidPlacedResult& vm);
[[nodiscard]] std::string
render_json_bid_placed_result(const BidPlacedResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const BidPlacedResult& vm);
[[nodiscard]] std::string render_outbid_telegram(int lot, resource_t amount,
                                                 CommodType type,
                                                 money_t new_bid,
                                                 std::string_view bidder_name,
                                                 player_t bidder);

/// Result of listing a commodity lot for sale (`sell <r|d|f|x> <amount>`).
struct SellResult {
  int lot{0};
  resource_t amount{0};
  CommodType type{CommodType::RESOURCE};
};

[[nodiscard]] std::string render_sell_result(const SellResult& vm);
[[nodiscard]] std::string render_json_sell_result(const SellResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const SellResult& vm);
[[nodiscard]] std::string render_sell_market_news(int lot, resource_t amount,
                                                  CommodType type,
                                                  std::string_view seller_name,
                                                  player_t seller);

/// View model for `treasury` command.
struct TreasuryViewModel {
  money_t total_income{0};
  money_t total_costs{0};
  money_t profit_market{0};
  money_t cost_market{0};
  money_t income_taxes{0};
  money_t cost_tech{0};
  money_t maintain{0};
  money_t money{0};
};

[[nodiscard]] std::string render_treasury(const TreasuryViewModel& vm);
[[nodiscard]] std::string render_json_treasury(const TreasuryViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TreasuryViewModel& vm);

/// Result of transferring ship ownership to an ally (`give <race> <#ship>`).
struct GiveResult {
  shipnum_t ship_number{0};
  std::string ship_desc{};
  player_t recipient_id{0};
  std::string recipient_name{};
  std::string orbit_path{};
};

[[nodiscard]] GiveResult make_give_result(const Ship& ship,
                                          player_t recipient_id,
                                          std::string_view recipient_name,
                                          std::string_view orbit_path);
[[nodiscard]] std::string render_give_result(const GiveResult& vm);
[[nodiscard]] std::string render_json_give_result(const GiveResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const GiveResult& vm);
[[nodiscard]] std::string
render_give_recipient_telegram(std::string_view donor_name, player_t donor_id,
                               std::string_view ship_desc,
                               std::string_view orbit_path);
[[nodiscard]] std::string render_give_news(std::string_view donor_name,
                                           player_t donor_id,
                                           std::string_view recipient_name,
                                           player_t recipient_id);

/// Result of paying funds from treasury to another race (`pay <race>
/// <amount>`).
struct PayResult {
  money_t amount{0};
  player_t recipient_id{0};
  std::string recipient_name{};
};

[[nodiscard]] std::string render_pay_result(const PayResult& vm);
[[nodiscard]] std::string render_json_pay_result(const PayResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const PayResult& vm);
[[nodiscard]] std::string
render_pay_recipient_telegram(std::string_view payer_name, player_t payer_id,
                              money_t amount);
[[nodiscard]] std::string render_pay_news(std::string_view payer_name,
                                          player_t payer_id,
                                          std::string_view recipient_name,
                                          player_t recipient_id);

/// Single active governor row in a `governors` table.
struct GovernorRow {
  governor_t governor{0};
  std::string name{};
  money_t money{0};
  std::string last_login{};
  std::optional<std::string> password{std::nullopt};
};

/// View model for `governors` command table.
struct GovernorsViewModel {
  bool is_leader{false};
  std::vector<GovernorRow> rows{};
};

[[nodiscard]] std::string render_governors(const GovernorsViewModel& vm);
[[nodiscard]] std::string render_json_governors(const GovernorsViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GovernorsViewModel& vm);

/// Result of changing a governor's password (`governors <gov> password <pw>`).
struct GovernorPasswordChangedResult {
  governor_t governor{0};
};

[[nodiscard]] std::string render_governor_password_changed_result(
    const GovernorPasswordChangedResult& vm);
[[nodiscard]] std::string render_json_governor_password_changed_result(
    const GovernorPasswordChangedResult& vm);
[[nodiscard]] std::string
render_for_mode(UiMode mode, const GovernorPasswordChangedResult& vm);

/// Result of appointing a governor (`appoint [<gov>] <password>`).
struct GovernorAppointedResult {
  governor_t governor{0};
  bool auto_assigned{false};
};

[[nodiscard]] std::string
render_governor_appointed_result(const GovernorAppointedResult& vm);
[[nodiscard]] std::string
render_json_governor_appointed_result(const GovernorAppointedResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GovernorAppointedResult& vm);

/// Result of revoking a governor (`revoke <gov> <password> [<target>]`).
struct GovernorRevokedResult {
  governor_t revoked_governor{0};
  governor_t target_governor{0};
  money_t money_transferred{0};
};

[[nodiscard]] std::string
render_governor_revoked_result(const GovernorRevokedResult& vm);
[[nodiscard]] std::string
render_json_governor_revoked_result(const GovernorRevokedResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GovernorRevokedResult& vm);
[[nodiscard]] std::string render_revoke_header_telegram(player_t player,
                                                        governor_t src_gov,
                                                        governor_t tgt_gov);
[[nodiscard]] std::string
render_revoke_star_telegram(std::string_view star_name);
[[nodiscard]] std::string render_revoke_ship_telegram(char ship_type_letter,
                                                      shipnum_t ship_number);
[[nodiscard]] std::string render_revoke_money_telegram(money_t transferred);
[[nodiscard]] std::string render_revoke_footer_telegram(player_t player,
                                                        governor_t src_gov);

/// Result of inviting a race to join an alliance block (`invite <race>`).
struct InviteResult {
  player_t host_id{0};
  std::string host_name{};
  std::string block_name{};
  player_t target_id{0};
  std::string target_name{};
};

[[nodiscard]] std::string render_invite_result(const InviteResult& vm);
[[nodiscard]] std::string render_json_invite_result(const InviteResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const InviteResult& vm);
[[nodiscard]] std::string
render_invite_target_notification(const InviteResult& vm);
[[nodiscard]] std::string
render_invite_declaration_news(const InviteResult& vm);

/// Result of blackballing/uninviting a race from an alliance block (`uninvite
/// <race>`).
struct UninviteResult {
  player_t host_id{0};
  std::string block_name{};
  player_t target_id{0};
  std::string target_name{};
};

[[nodiscard]] std::string render_uninvite_result(const UninviteResult& vm);
[[nodiscard]] std::string render_json_uninvite_result(const UninviteResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const UninviteResult& vm);
[[nodiscard]] std::string
render_uninvite_target_notification(const UninviteResult& vm);
[[nodiscard]] std::string
render_uninvite_declaration_news(const UninviteResult& vm);

/// Result of pledging allegiance to an alliance block (`pledge <race>`).
struct PledgeResult {
  player_t pledger_id{0};
  std::string pledger_name{};
  player_t block_id{0};
  std::string block_name{};
  bool bandwagon{false};
};

[[nodiscard]] std::string render_pledge_result(const PledgeResult& vm);
[[nodiscard]] std::string render_json_pledge_result(const PledgeResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const PledgeResult& vm);
[[nodiscard]] std::string
render_pledge_block_notification(const PledgeResult& vm);
[[nodiscard]] std::string
render_pledge_self_notification(const PledgeResult& vm);
[[nodiscard]] std::string
render_pledge_declaration_news(const PledgeResult& vm);

/// Result of withdrawing pledge from an alliance block (`unpledge <race>`).
struct UnpledgeResult {
  player_t pledger_id{0};
  std::string pledger_name{};
  player_t block_id{0};
  std::string block_name{};
  bool taunt{false};
};

[[nodiscard]] std::string render_unpledge_result(const UnpledgeResult& vm);
[[nodiscard]] std::string render_json_unpledge_result(const UnpledgeResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const UnpledgeResult& vm);
[[nodiscard]] std::string
render_unpledge_block_notification(const UnpledgeResult& vm);
[[nodiscard]] std::string
render_unpledge_self_notification(const UnpledgeResult& vm);
[[nodiscard]] std::string
render_unpledge_declaration_news(const UnpledgeResult& vm);

/// Diplomatic stance declared via `declare <race> <alliance|neutral|war>`.
enum class DiplomaticRelation : std::uint8_t {
  Alliance,
  Neutral,
  War,
};

/// News bulletin flavor variant when declaring war (`declare <race> war`).
enum class WarDeclarationNewsVariant : std::uint8_t {
  Standard,
  HadEnough,
  TimeToDeclare,
  NoChoice,
  ScrewIt,
};

inline constexpr std::array all_war_declaration_news_variants = {
    WarDeclarationNewsVariant::Standard,
    WarDeclarationNewsVariant::HadEnough,
    WarDeclarationNewsVariant::TimeToDeclare,
    WarDeclarationNewsVariant::NoChoice,
    WarDeclarationNewsVariant::ScrewIt,
};

/// Result of declaring diplomatic relations (`declare <race>
/// <alliance|neutral|war> [<mod>]`).
struct DeclareResult {
  player_t declarer_id{0};
  std::string declarer_name{};
  player_t target_id{0};
  std::string target_name{};
  DiplomaticRelation relation{DiplomaticRelation::Neutral};
  bool rare_quip{false};
  WarDeclarationNewsVariant news_variant{WarDeclarationNewsVariant::Standard};
  int translation_modifier{30};
};

[[nodiscard]] std::string render_declare_result(const DeclareResult& vm);
[[nodiscard]] std::string render_json_declare_result(const DeclareResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const DeclareResult& vm);
[[nodiscard]] std::string
render_declare_target_notification(const DeclareResult& vm);
[[nodiscard]] std::string
render_declare_declaration_news(const DeclareResult& vm);

/// Result of querying or designating a governmental center (`capital
/// [<ship>]`).
struct CapitalResult {
  shipnum_t shipno{0};
  bool designated{false};
  double efficiency{0.0};
};

[[nodiscard]] std::string render_capital_result(const CapitalResult& vm);
[[nodiscard]] std::string render_json_capital_result(const CapitalResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const CapitalResult& vm);

/// Voter role classification when inspecting update votes (`vote`).
enum class VoteViewerRole : std::uint8_t {
  Mortal,
  God,
  Guest,
};

/// Single race's vote entry visible to deities in `vote`.
struct RaceVoteEntry {
  std::string race_name{};
  bool vote_go{false};
};

/// View model for `vote` query output.
struct VoteViewModel {
  VoteViewerRole viewer_role{VoteViewerRole::Mortal};
  bool own_vote_go{false};
  int total_votes{0};
  int yays{0};
  int nays{0};
  std::vector<RaceVoteEntry> details{};
};

[[nodiscard]] std::string render_vote(const VoteViewModel& vm);
[[nodiscard]] std::string render_json_vote(const VoteViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const VoteViewModel& vm);

/// Result of casting an update vote (`vote update <go|wait>`).
struct VoteCastResult {
  player_t player{0};
  bool vote_go{false};
  bool unanimous{false};
};

[[nodiscard]] std::string render_vote_cast_result(const VoteCastResult& vm);
[[nodiscard]] std::string
render_json_vote_cast_result(const VoteCastResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const VoteCastResult& vm);

/// Result of dissolving an empire (`dissolve <race_pw> <leader_pw> [waste]`).
struct DissolveResult {
  player_t player{0};
  std::string race_name{};
  bool waste{false};
  std::vector<shipnum_t> destroyed_ships{};
};

[[nodiscard]] std::string render_dissolve_result(const DissolveResult& vm);
[[nodiscard]] std::string render_json_dissolve_result(const DissolveResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DissolveResult& vm);
[[nodiscard]] std::string
render_dissolve_leader_warning(governor_t attempting_governor);
[[nodiscard]] std::string
render_dissolve_declaration_news(const DissolveResult& vm);

/// Entity category targeted by deity `fix` command.
enum class FixTarget : std::uint8_t {
  Planet,
  Ship,
};

/// Result of inspecting or overriding a planet or ship property via `fix`.
struct FixResult {
  FixTarget target{FixTarget::Planet};
  std::string property{};
  std::string display_property{};
  double value{0.0};
  bool ship_destroyed{false};
  std::string ship_label{};
};

[[nodiscard]] std::string render_fix_result(const FixResult& vm);
[[nodiscard]] std::string render_json_fix_result(const FixResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const FixResult& vm);

/// Entity category targeted by `name` command.
enum class NameTarget : std::uint8_t {
  Ship,
  Class,
  Block,
  Star,
  Planet,
  Race,
  Governor,
};

/// Result of naming or renaming an entity via `name`.
struct NameResult {
  NameTarget target{NameTarget::Ship};
  std::string name{};
};

[[nodiscard]] std::string render_name_result(const NameResult& vm);
[[nodiscard]] std::string render_json_name_result(const NameResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const NameResult& vm);

/// Formats a race/governor sender tag (`<Race> "<Gov>" [<player>,<gov>]`).
[[nodiscard]] std::string format_sender_tag(const Race& race, player_t player,
                                            governor_t governor);

/// Recipient category targeted by `send` command.
enum class SendMessageTarget : std::uint8_t {
  Player,
  Block,
  Star,
};

/// Result of dispatching a telegram, block message, or stargram via `send`.
struct SendMessageResult {
  SendMessageTarget target{SendMessageTarget::Player};
};

[[nodiscard]] std::string
render_send_message_result(const SendMessageResult& vm);
[[nodiscard]] std::string
render_json_send_message_result(const SendMessageResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const SendMessageResult& vm);

/// Result of posting a public bulletin via `post`.
struct PostResult {
  std::string sender{};
  std::string message{};
};

[[nodiscard]] std::string render_post_result(const PostResult& vm);
[[nodiscard]] std::string render_json_post_result(const PostResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const PostResult& vm);

/// Live communication channel used by `announce`, `broadcast`, `shout`, and
/// `think`.
enum class AnnounceMode : std::uint8_t {
  Announce,
  Broadcast,
  Shout,
  Think,
};

/// Result of dispatching a live communication via `announce` / `broadcast` /
/// `shout` / `think`.
struct AnnounceResult {
  AnnounceMode mode{AnnounceMode::Announce};
  std::string sender{};
  std::string message{};
};

[[nodiscard]] std::string
render_announce_dispatch_message(const AnnounceResult& vm);
[[nodiscard]] std::string render_announce_result(const AnnounceResult& vm);
[[nodiscard]] std::string render_json_announce_result(const AnnounceResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const AnnounceResult& vm);

/// Recipient category targeted by `page` command.
enum class PageTarget : std::uint8_t {
  Player,
  Block,
};

/// Result of paging a player or alliance block via `page`.
struct PageResult {
  PageTarget target{PageTarget::Player};
};

[[nodiscard]] std::string render_page_notification(std::string_view race_name,
                                                   std::string_view gov_name,
                                                   std::string_view star_name);
[[nodiscard]] std::string render_page_result(const PageResult& vm);
[[nodiscard]] std::string render_json_page_result(const PageResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const PageResult& vm);

}  // namespace GB::presentation
