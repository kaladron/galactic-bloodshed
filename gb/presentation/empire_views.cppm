// SPDX-License-Identifier: Apache-2.0

/// \file empire_views.cppm
/// \brief Empire, diplomatic, and player status view models and renderers.

export module gb.presentation.empire_views;

export import gb.presentation.core;
import gb.presentation.planet_views;

import gb.entities;
import gb.services;
import std;

export namespace GB::presentation {

/// Diplomatic stance declared via `declare <race> <alliance|neutral|war>`
/// or between two empires.
enum class DiplomaticRelation : std::uint8_t {
  Neutral,
  Allied,
  AtWar,
  Alliance = Allied,
  War = AtWar,
};

/// Fog-of-war filtered biological and technological characteristics of a race.
/// Numeric values use std::int64_t when quantized through
/// Race::estimate_value(), or exact values when viewer.can_see(target_player ==
/// viewer.player).
struct RaceStatsView {
  std::optional<bool> is_metamorph{std::nullopt};
  std::optional<std::int64_t> fertilize{std::nullopt};
  std::optional<std::int64_t> temp{std::nullopt};
  std::optional<double> birthrate{std::nullopt};
  std::optional<double> mass{std::nullopt};
  std::optional<std::int64_t> fighters{std::nullopt};
  std::optional<double> metabolism{std::nullopt};
  std::optional<std::int64_t> number_sexes{std::nullopt};
  std::optional<double> adventurism{std::nullopt};
  std::optional<std::int64_t> iq{std::nullopt};
  std::optional<double> tech{std::nullopt};
  std::optional<std::int64_t> morale{std::nullopt};
  std::optional<std::int64_t> gun_range{std::nullopt};
  std::optional<std::int64_t> space_tele_range{std::nullopt};
  std::optional<std::int64_t> ground_tele_range{std::nullopt};
  std::optional<ConditionValues> conditions{std::nullopt};
  std::optional<SectorType> best_sector{std::nullopt};
};

/// Fog-of-war filtered empire power and stockpile metrics.
struct RacePowerStandingsView {
  std::optional<int> rank{std::nullopt};
  std::optional<std::int64_t> victory_score{std::nullopt};
  std::optional<std::int64_t> troops{std::nullopt};
  std::optional<std::int64_t> popn{std::nullopt};
  std::optional<std::int64_t> money{std::nullopt};
  std::optional<std::int64_t> ships{std::nullopt};
  std::optional<std::int64_t> planets{std::nullopt};
  std::optional<std::int64_t> resource{std::nullopt};
  std::optional<std::int64_t> fuel{std::nullopt};
  std::optional<std::int64_t> destruct{std::nullopt};
};

/// Active governor summary visible to the owning empire (or deity).
struct GovernorSummaryView {
  governor_t id{0};
  std::string name{};
  money_t money{0};
  money_t income{0};
  money_t maintain{0};
  money_t cost_tech{0};
  money_t cost_market{0};
  money_t profit_market{0};
  std::time_t last_login{0};
  ScopeLevel default_level{ScopeLevel::LEVEL_UNIV};
  starnum_t default_star{0};
  planetnum_t default_planet{0};
  starnum_t home_star{0};
  planetnum_t home_planet{0};
};

/// Private empire details visible only when viewer.can_see(target ==
/// viewer.player).
struct RaceSelfDetailsView {
  std::optional<shipnum_t> gov_ship{std::nullopt};
  turn_t updates_active{0};
  SectorCompatibilities sector_compat{};
  TechDiscoveries discoveries{};
  bool vote_update_go{false};
  std::vector<GovernorSummaryView> governors{};
};

/// Canonical view model for a race/empire ("type": "race").
struct RaceView {
  player_t player{0};
  std::string name{};
  bool dissolved{false};
  bool is_god{false};
  bool is_guest{false};
  std::string personal_info{};
  Percentage know_pct{0};
  DiplomaticRelation you_to_them{DiplomaticRelation::Neutral};
  DiplomaticRelation them_to_you{DiplomaticRelation::Neutral};
  RaceStatsView stats{};
  std::optional<RacePowerStandingsView> standings{std::nullopt};
  std::optional<RaceSelfDetailsView> self{std::nullopt};
};

/// Canonical view model for an alliance block ("type": "block").
struct BlockView {
  player_t block_id{0};
  std::string name{};
  std::string motto{};
  std::vector<player_t> members{};
  std::vector<player_t> invited{};
  std::vector<player_t> pledged{};
  Percentage know_pct{0};
  std::optional<RacePowerStandingsView> standings{std::nullopt};
};

/// Canonical view model for a commodity market lot ("type": "market_lot").
struct MarketLotView {
  commodnum_t id{0};
  bool active{true};
  CommodType type{CommodType::RESOURCE};
  resource_t amount{0};
  player_t owner{0};
  player_t bidder{0};
  money_t bid{0};
  double cost_per_unit{0.0};
  money_t shipping_cost{0};
  std::optional<starnum_t> dest_star{std::nullopt};
  std::optional<planetnum_t> dest_planet{std::nullopt};
};

/// Projects canonical RaceView under viewer scope and fog-of-war rules.
[[nodiscard]] RaceView
project_race_view(EntityManager& em, const ScopeContext& viewer,
                  player_t target_player,
                  std::optional<int> rank = std::nullopt);

/// Projects canonical BlockView under viewer scope and fog-of-war rules.
[[nodiscard]] BlockView project_block_view(EntityManager& em,
                                           const ScopeContext& viewer,
                                           player_t block_id);

/// Projects canonical MarketLotView under viewer scope and fog-of-war rules.
[[nodiscard]] MarketLotView project_market_lot_view(
    EntityManager& em, const ScopeContext& viewer, const Commod& commod,
    std::optional<starnum_t> shipping_dest_star = std::nullopt);

/// Projects a tombstone MarketLotView representing a completed/deleted market
/// lot.
[[nodiscard]] MarketLotView project_deleted_market_lot_view(commodnum_t id);

/// Formats a canonical RaceView into its JSON envelope string.
[[nodiscard]] std::string render_json_race_view(const RaceView& vm);

/// Formats a canonical BlockView into its JSON envelope string.
[[nodiscard]] std::string render_json_block_view(const BlockView& vm);

/// Formats a canonical MarketLotView into its JSON envelope string.
[[nodiscard]] std::string render_json_market_lot_view(const MarketLotView& vm);

/// Renders all modified entities captured during a JSON command execution
/// into their canonical JSON envelopes.
[[nodiscard]] std::string
render_dirty_entities_json(EntityManager& em, const ScopeContext& viewer,
                           const DirtyEntitySet& dirty);

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
  std::vector<RaceView> races{};
};

/// Renders a `RelationViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_relation(const RelationViewModel& vm);

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
  std::vector<RaceView> races{};
  std::vector<ErrorView> errors{};
};

/// Renders a `WhoisViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_whois(const WhoisViewModel& vm);

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
  RaceView race{};
};

/// Renders a `ProfileSelfViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_profile_self(const ProfileSelfViewModel& vm);

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
  RaceView race{};
};

/// Renders a `ProfileOtherViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_profile_other(const ProfileOtherViewModel& vm);

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
  std::vector<BlockView> block_views{};
  std::vector<RaceView> member_views{};
};

/// Renders a `BlockViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_block(const BlockViewModel& vm);

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
  std::vector<PlanetView> planets{};
};

/// Renders a `ColoniesViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_colonies(const ColoniesViewModel& vm);

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
  std::vector<RaceView> races{};
};

/// Renders a `PowerViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_power(const PowerViewModel& vm);

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
  std::vector<PlanetView> planets{};
  population_t total_popn{0};
  money_t total_invest{0};
  double total_gain{0.0};
  double total_max_gain{0.0};
};

/// Renders a `TechStatusViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_tech_status(const TechStatusViewModel& vm);

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
  std::vector<RaceView> races{};
};

/// Renders a `VictoryViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_victory(const VictoryViewModel& vm);

/// Dispatches `VictoryViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const VictoryViewModel& vm);

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
  std::vector<MarketLotView> market_lots{};
};

[[nodiscard]] std::string
render_commodity_market(const CommodityMarketViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const CommodityMarketViewModel& vm);

/// Result of placing a bid on a commodity lot (`bid <lot> <bid>`).
struct BidPlacedResult {
  int lot{0};
  money_t bid{0};
  money_t shipping_cost{0};
};

[[nodiscard]] std::string render_bid_placed_result(const BidPlacedResult& vm);
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
  RaceView race{};
};

[[nodiscard]] std::string render_treasury(const TreasuryViewModel& vm);
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
  RaceView race{};
};

[[nodiscard]] std::string render_governors(const GovernorsViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GovernorsViewModel& vm);

/// Result of appointing a governor (`appoint [<gov>] <password>`).
struct GovernorAppointedResult {
  governor_t governor{0};
  bool auto_assigned{false};
};

[[nodiscard]] std::string
render_governor_appointed_result(const GovernorAppointedResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GovernorAppointedResult& vm);

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

[[nodiscard]] std::string
render_unpledge_block_notification(const UnpledgeResult& vm);
[[nodiscard]] std::string
render_unpledge_self_notification(const UnpledgeResult& vm);
[[nodiscard]] std::string
render_unpledge_declaration_news(const UnpledgeResult& vm);

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
[[nodiscard]] std::string render_for_mode(UiMode mode, const DeclareResult& vm);
[[nodiscard]] std::string
render_declare_target_notification(const DeclareResult& vm);
[[nodiscard]] std::string
render_declare_declaration_news(const DeclareResult& vm);

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
  std::vector<RaceView> races{};
};

[[nodiscard]] std::string render_vote(const VoteViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const VoteViewModel& vm);

/// Result of dissolving an empire (`dissolve <race_pw> <leader_pw> [waste]`).
struct DissolveResult {
  player_t player{0};
  std::string race_name{};
  bool waste{false};
  std::vector<shipnum_t> destroyed_ships{};
};

[[nodiscard]] std::string render_dissolve_result(const DissolveResult& vm);
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
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const SendMessageResult& vm);

/// Result of posting a public bulletin via `post`.
struct PostResult {
  std::string sender{};
  std::string message{};
};

[[nodiscard]] std::string render_post_result(const PostResult& vm);
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
[[nodiscard]] std::string render_for_mode(UiMode mode, const PageResult& vm);

/// Single timestamped telegram or news entry in a `read` view model.
struct TimestampedMessage {
  std::int64_t id{0};
  std::int64_t timestamp{0};
  std::string message{};
};

/// View model for `read` / `read telegram`.
struct TelegramReadResult {
  std::vector<TimestampedMessage> messages{};
};

[[nodiscard]] TelegramReadResult
make_telegram_read_result(std::vector<TelegramItem> items);
[[nodiscard]] std::string
render_telegram_read_result(const TelegramReadResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TelegramReadResult& vm);

/// View model for `read news`.
struct NewsBundleResult {
  std::vector<TimestampedMessage> declarations{};
  std::vector<TimestampedMessage> combat{};
  std::vector<TimestampedMessage> business{};
  std::vector<TimestampedMessage> bulletins{};
};

[[nodiscard]] NewsBundleResult make_news_bundle_result(
    std::vector<NewsItem> declarations, std::vector<NewsItem> combat,
    std::vector<NewsItem> business, std::vector<NewsItem> bulletins);
[[nodiscard]] std::string render_news_bundle_result(const NewsBundleResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const NewsBundleResult& vm);

/// Attribute, flag, or stockpile granted or adjusted by deity `bless`.
enum class BlessProperty : std::uint8_t {
  Password,
  Pods,
  NoPods,
  CollectiveIq,
  NoCollectiveIq,
  Guest,
  God,
  Mortal,
  Water,
  Land,
  Mountain,
  Gas,
  Ice,
  Forest,
  Desert,
  Plated,
  Money,
  Morale,
  Fertility,
  Iq,
  Fight,
  Technology,
  MaxIq,
  Mass,
  Metabolism,
  Adventurism,
  Birthrate,
  ExploreBit,
  NoExploreBit,
  PlanetPopulation,
  Inhabited,
  NumSectsOwned,
  Resources,
  Destruct,
  Fuel,
  Crystals,
  ActionPoints,
};

/// Typed payload for a `bless` operation (`std::monostate` for flag toggles).
using BlessValue = std::variant<std::monostate, int, double, std::string>;

/// Result of bestowing a divine blessing upon a player via `bless`.
struct BlessResult {
  player_t target_player{0};
  BlessProperty property{BlessProperty::Money};
  BlessValue value{};
  std::string star_name{};
  std::string planet_name{};
};

[[nodiscard]] std::string render_bless_notification(const BlessResult& vm);

/// View model for `help [<topic>]` documentation output.
struct HelpViewModel {
  std::string topic{};
  bool is_index{false};
  std::string content{};
};

[[nodiscard]] std::string render_help(const HelpViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const HelpViewModel& vm);

/// Turn bulletin payload when a commodity market lot settles.
struct MarketLotSettlementNotice {
  int lot{0};
  CommodType type{CommodType::RESOURCE};
  resource_t amount{0};
  money_t bid{0};
  player_t seller{0};
  std::string seller_name{};
  player_t buyer{0};
  std::string buyer_name{};
  std::string star_name{};
  std::string planet_name{};
};

/// Formats the buyer's delivery telegram when a market lot settles.
[[nodiscard]] std::string
render_market_purchase_telegram(const MarketLotSettlementNotice& vm);

/// Formats the seller's proceeds telegram when a market lot settles.
[[nodiscard]] std::string
render_market_sale_telegram(const MarketLotSettlementNotice& vm);

/// Formats the empire-wide bulletin when a star starts or finishes going nova.
[[nodiscard]] std::string render_star_nova_bulletin(std::string_view star_name,
                                                    bool started);

/// Single winner entry in an end-of-game victory announcement.
struct VictoryWinnerEntry {
  player_t player{0};
  std::string name{};
};

/// View model for the end-of-game victory broadcast telegram.
struct VictoryAnnouncementViewModel {
  std::vector<VictoryWinnerEntry> big_winners{};
  std::vector<VictoryWinnerEntry> lesser_winners{};
};

/// Formats the cohesive end-of-game victory broadcast telegram.
[[nodiscard]] std::string
render_victory_announcement(const VictoryAnnouncementViewModel& vm);

/// Governor configuration flag modified by `toggle <option>`.
enum class ToggleOption : std::uint8_t {
  Inverse,
  Color,
  DoubleDigits,
  Geography,
  Gag,
  Autoload,
  Invisible,
  Compatibility,
};

/// View model for `toggle` (display all governor configuration options).
struct ToggleStatusViewModel {
  toggletype toggles{};
  bool color{false};
};
[[nodiscard]] std::string render_toggle_status(const ToggleStatusViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ToggleStatusViewModel& vm);

/// Result of toggling a single governor configuration option (`toggle <opt>`).
struct ToggleChangedResult {
  ToggleOption option{ToggleOption::Gag};
  bool enabled{false};
};
[[nodiscard]] std::string
render_toggle_changed_result(const ToggleChangedResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ToggleChangedResult& vm);

}  // namespace GB::presentation
