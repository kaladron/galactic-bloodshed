// SPDX-License-Identifier: Apache-2.0

/// \file planet_views.cppm
/// \brief Planetary and stellar view models and presentation declarations.

export module gb.presentation.planet_views;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import gb.mechanics;
import std;

export namespace GB::presentation {

/// Landed ship entry on a surveyed planetary sector.
struct SurveySectorShip {
  shipnum_t shipno{0};
  char ltr{' '};
  player_t owner{0};
};

/// Single sector row in a planetary sector-range survey.
struct SurveySectorRow {
  Coordinates coords{0, 0};
  char sect_char{' '};
  char type_char{' '};
  char desshow_char{' '};
  bool wasted{false};
  player_t owner{0};
  Percentage eff{0};
  Percentage fert{0};
  Percentage mobilization{0};
  bool crystals{false};
  resource_t resource{0};
  population_t popn{0};
  population_t troops{0};
  population_t maxpopn{0};
  std::vector<SurveySectorShip> ships{};
};

/// View model for planetary sector-range surveys (`survey -` or
/// `survey x:x,y:y`).
struct SurveySectorViewModel {
  Coordinates dimensions{0, 0};
  std::string star_name{};
  std::string planet_name{};
  resource_t resource_stock{0};
  resource_t fuel_stock{0};
  resource_t destruct_stock{0};
  population_t popn{0};
  population_t maxpopn{0};
  Percentage toxicity{0};
  double compatibility{0.0};
  player_t enslaved_to{0};
  std::vector<SurveySectorRow> sectors{};
};

/// View model for planetary overview surveys (`survey` at planet scope).
struct SurveyPlanetOverviewViewModel {
  std::string planet_name{};
  std::string star_name{};
  double gravity{0.0};
  UniverseCoordinates absolute_coords{0.0, 0.0};
  SystemCoordinates system_coords{0.0, 0.0};
  ConditionValues planet_conditions{};
  ConditionValues race_conditions{};
  temperature_t planet_temp{0};
  temperature_t race_temp{0};
  temperature_t normal_temp{0};
  Percentage toxicity{0};
  double compatibility{0.0};
  Percentage avg_fertility{0};
  resource_t avg_resource{0};
  sector_count_t crystal_sectors{0};
  resource_t total_resources{0};
  bool is_metamorph{false};
  resource_t fuel_stock{0};
  resource_t resource_stock{0};
  resource_t destruct_stock{0};
  population_t popn{0};
  population_t maxpopn{0};
  std::optional<player_t> slaved_to{std::nullopt};
};

/// View model for stellar system surveys (`survey` at star scope).
struct SurveyStarViewModel {
  std::string star_name{};
  UniverseCoordinates coordinates{0.0, 0.0};
  bool is_god{false};
  double gravity{0.0};
  std::optional<int> stability{std::nullopt};
  int temperature_class{0};
  std::vector<std::string> planet_names{};
};

/// View model for universe-scope surveys (`survey` at universe scope).
struct SurveyUniverseViewModel {
  std::string message{"It's just _there_, you know?"};
};

/// Star entry in an orbital system/universe map.
struct OrbitStarEntry {
  SystemCoordinates coords{0.0, 0.0};
  bool symbol_standout{false};
  bool label_standout{false};
  std::string label{};
};

/// Planet entry in an orbital system/planet map.
struct OrbitPlanetEntry {
  SystemCoordinates coords{0.0, 0.0};
  char symbol{'?'};
  bool symbol_standout{false};
  bool label_standout{false};
  std::string label{};
  std::optional<Percentage> compatibility{std::nullopt};
};

/// Ship entry in an orbital map.
struct OrbitShipEntry {
  SystemCoordinates coords{0.0, 0.0};
  bearing_t mirror_heading{0};
  char symbol{' '};
  bool standout{false};
  shipnum_t shipno{0};
};

/// View model for `orbit` command maps (universe, star system, or planet
/// orbit).
struct OrbitViewModel {
  bool inverse{false};
  std::vector<OrbitStarEntry> stars{};
  std::vector<OrbitPlanetEntry> planets{};
  std::vector<OrbitShipEntry> ships{};
};

/// Renders a `PlanetMapViewModel` as a 2D ASCII planetary map with X/Y
/// coordinate headers, ANSI SGR reverse-video (`\x1b[7m...\x1b[27m`) for
/// `inverse` cells, and a `tabulate::Table` for planetary statistics.
[[nodiscard]] std::string render_ascii_planet_map(const PlanetMapViewModel& vm);

/// Serializes a `PlanetMapViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"map","data":{...}}\n`).
[[nodiscard]] std::string render_json_planet_map(const PlanetMapViewModel& vm);

/// Dispatches `PlanetMapViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PlanetMapViewModel& vm);

/// Renders a `SurveySectorViewModel` as a `tabulate::Table` for
/// `UiMode::ASCII`.
[[nodiscard]] std::string
render_survey_sectors(const SurveySectorViewModel& vm);

/// Serializes a `SurveySectorViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"survey_sectors","data":{...}}\n`).
[[nodiscard]] std::string
render_json_survey_sectors(const SurveySectorViewModel& vm);

/// Dispatches `SurveySectorViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const SurveySectorViewModel& vm);

/// Renders a `SurveyPlanetOverviewViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string
render_survey_planet_overview(const SurveyPlanetOverviewViewModel& vm);

/// Serializes a `SurveyPlanetOverviewViewModel` into a newline-delimited Glaze
/// JSON envelope (`{"type":"survey_planet","data":{...}}\n`).
[[nodiscard]] std::string
render_json_survey_planet_overview(const SurveyPlanetOverviewViewModel& vm);

/// Dispatches `SurveyPlanetOverviewViewModel` rendering based on `mode`.
[[nodiscard]] std::string
render_for_mode(UiMode mode, const SurveyPlanetOverviewViewModel& vm);

/// Renders a `SurveyStarViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_survey_star(const SurveyStarViewModel& vm);

/// Serializes a `SurveyStarViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"survey_star","data":{...}}\n`).
[[nodiscard]] std::string
render_json_survey_star(const SurveyStarViewModel& vm);

/// Dispatches `SurveyStarViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const SurveyStarViewModel& vm);

/// Renders a `SurveyUniverseViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string
render_survey_universe(const SurveyUniverseViewModel& vm);

/// Serializes a `SurveyUniverseViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"survey_universe","data":{...}}\n`).
[[nodiscard]] std::string
render_json_survey_universe(const SurveyUniverseViewModel& vm);

/// Dispatches `SurveyUniverseViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const SurveyUniverseViewModel& vm);

/// Planet row in an `explore` star system table.
struct ExplorePlanetRow {
  planetnum_t planet_order{0};
  std::string planet_name{};
  bool explored{false};
  bool autorep{false};
  bool inhabited{false};
  bool enslaved{false};
  std::vector<player_t> alien_owners{};
  bool toxic{false};
  std::string type_name{};
  double compatibility{0.0};
};

/// Star system entry in an `explore` report.
struct ExploreStarEntry {
  std::string star_name{};
  std::optional<int> stability{std::nullopt};
  ap_t ap{0};
  std::vector<ExplorePlanetRow> planets{};
};

/// View model for `explore` command global or star-specific census reports.
struct ExploreViewModel {
  ap_t global_ap{0};
  std::vector<ExploreStarEntry> stars{};
};

/// Renders an `OrbitViewModel` as a 2D ASCII orbital canvas with object legend
/// and ANSI SGR reverse-video (`\x1b[7m...\x1b[27m`) for `inverse` standout.
[[nodiscard]] std::string render_ascii_orbit_map(const OrbitViewModel& vm);

/// Serializes an `OrbitViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"orbit","data":{...}}\n`).
[[nodiscard]] std::string render_json_orbit_map(const OrbitViewModel& vm);

/// Dispatches `OrbitViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const OrbitViewModel& vm);

/// Renders an `ExploreViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_explore(const ExploreViewModel& vm);

/// Serializes an `ExploreViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"explore","data":{...}}\n`).
[[nodiscard]] std::string render_json_explore(const ExploreViewModel& vm);

/// Dispatches `ExploreViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ExploreViewModel& vm);

/// Single ranked sector entry in a planetary `analysis` category list.
struct AnalysisSectorEntry {
  Coordinates coords{0, 0};
  char sector_char{'.'};
  resource_t value{0};
};

/// Single player, unowned, or total summary row in a planetary `analysis`
/// table.
struct AnalysisStatsRow {
  std::string label{};
  int sectors{0};
  population_t popn{0};
  population_t troops{0};
  double avg_eff{0.0};
  double avg_mob{0.0};
  resource_t res{0};
  int crystals{0};
  std::array<int, SectorType::SEC_WASTED + 1> by_type{};
};

/// Planetary report entry in an `analysis` view model.
struct AnalysisPlanetReport {
  std::string star_name{};
  std::string planet_name{};
  bool is_top_five{true};
  int care_count{5};
  std::optional<std::string> sector_type_name{std::nullopt};
  std::string filter_description{};
  std::vector<AnalysisSectorEntry> troops{};
  std::vector<AnalysisSectorEntry> res{};
  std::vector<AnalysisSectorEntry> eff{};
  std::vector<AnalysisSectorEntry> frt{};
  std::vector<AnalysisSectorEntry> mob{};
  std::vector<AnalysisSectorEntry> popn{};
  std::vector<AnalysisSectorEntry> m_popn{};
  std::vector<AnalysisStatsRow> rows{};
};

/// View model for `analysis` command reports (planet or star scope).
struct AnalysisViewModel {
  std::vector<AnalysisPlanetReport> planets{};
};

/// Renders an `AnalysisViewModel` for `UiMode::ASCII`.
[[nodiscard]] std::string render_analysis(const AnalysisViewModel& vm);

/// Serializes an `AnalysisViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"analysis","data":{...}}\n`).
[[nodiscard]] std::string render_json_analysis(const AnalysisViewModel& vm);

/// Dispatches `AnalysisViewModel` rendering based on `mode`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const AnalysisViewModel& vm);

/// View model for querying current planetary mobilization (`mobilize`).
struct MobilizeStatusViewModel {
  Percentage current_mobilization{0};
  Percentage quota{0};
};

/// Result of setting planetary mobilization quota (`mobilize <percentage>`).
struct MobilizeSetResult {
  Percentage quota{0};
};

/// View model for querying planetary tax rate (`tax`).
struct TaxStatusViewModel {
  Percentage current_rate{0};
  Percentage target_rate{0};
};

/// Result of setting planetary tax rate (`tax <rate>`).
struct TaxSetResult {
  Percentage target_rate{0};
};

/// Result of setting planetary toxicity threshold (`toxicity <threshold>`).
struct ToxicityResult {
  Percentage threshold{0};
};

/// Result of allocating global action points to a star system (`allocate`).
struct AllocateResult {
  ap_t allocated{0};
  ap_t star_ap{0};
  ap_t global_ap{0};
};

/// Result of repairing wasted planetary sectors (`repair`).
struct RepairResult {
  int sectors_repaired{0};
  resource_t resource_cost{0};
};

/// Result of attempting to enslave a planet with an Orbiting Assault Platform
/// (`enslave <ship>`).
struct EnslaveResult {
  bool enslaved{false};
  std::string orbit_path{};
  std::string enslaver_race_name{};
  resource_t attack_strength{0};
  resource_t defense_strength{0};
  double required_garrison{0.0};
};

[[nodiscard]] std::string
render_mobilize_status(const MobilizeStatusViewModel& vm);
[[nodiscard]] std::string
render_json_mobilize_status(const MobilizeStatusViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MobilizeStatusViewModel& vm);

[[nodiscard]] std::string
render_mobilize_set_result(const MobilizeSetResult& vm);
[[nodiscard]] std::string
render_json_mobilize_set_result(const MobilizeSetResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MobilizeSetResult& vm);

[[nodiscard]] std::string render_tax_status(const TaxStatusViewModel& vm);
[[nodiscard]] std::string render_json_tax_status(const TaxStatusViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TaxStatusViewModel& vm);

[[nodiscard]] std::string render_tax_set_result(const TaxSetResult& vm);
[[nodiscard]] std::string render_json_tax_set_result(const TaxSetResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const TaxSetResult& vm);

[[nodiscard]] std::string render_toxicity_result(const ToxicityResult& vm);
[[nodiscard]] std::string render_json_toxicity_result(const ToxicityResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ToxicityResult& vm);

[[nodiscard]] std::string render_allocate_result(const AllocateResult& vm);
[[nodiscard]] std::string render_json_allocate_result(const AllocateResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const AllocateResult& vm);

[[nodiscard]] std::string render_repair_result(const RepairResult& vm);
[[nodiscard]] std::string render_json_repair_result(const RepairResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const RepairResult& vm);

[[nodiscard]] std::string render_enslave_result(const EnslaveResult& vm);
[[nodiscard]] std::string render_json_enslave_result(const EnslaveResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const EnslaveResult& vm);
[[nodiscard]] std::string render_enslave_telegram(std::string_view star_name,
                                                  std::string_view planet_name,
                                                  std::string_view ship_desc,
                                                  const EnslaveResult& vm);

/// Result of transferring planetary stockpiles to another race (`transfer`).
struct TransferResult {
  std::string star_name{};
  std::string planet_name{};
  CommodType type{CommodType::RESOURCE};
  resource_t amount{0};
  player_t sender{0};
  player_t recipient{0};
};

[[nodiscard]] std::string render_transfer_result(const TransferResult& vm);
[[nodiscard]] std::string render_json_transfer_result(const TransferResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TransferResult& vm);

/// Result of financing a planetary insurgency (`insurgency <race> <money>`).
struct InsurgencyResult {
  std::string star_name{};
  std::string planet_name{};
  player_t instigator_id{0};
  std::string instigator_name{};
  player_t target_id{0};
  std::string target_name{};
  population_t instigator_system_pop{0};
  population_t target_system_pop{0};
  long instigator_morale{0};
  long target_morale{0};
  money_t money{0};
  population_t target_planet_pop{0};
  int target_tax_rate{0};
  double raw_factor{0.0};
  int success_chance{0};
  bool succeeded{false};
  int sectors_changed{0};
};

[[nodiscard]] std::string render_insurgency_result(const InsurgencyResult& vm);
[[nodiscard]] std::string
render_json_insurgency_result(const InsurgencyResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const InsurgencyResult& vm);
[[nodiscard]] std::string
render_insurgency_telegram(const InsurgencyResult& vm);
[[nodiscard]] std::string render_insurgency_news(const InsurgencyResult& vm);

/// Single active shipping route entry for a planet (`route` / `route <n>`).
struct RouteRow {
  int route_num{0};
  Coordinates dest_coords{0, 0};
  std::string load{};
  std::string unload{};
  std::string dest_star_name{};
  std::string dest_planet_name{};
};

/// View model for planetary shipping route inspection (`route` / `route <n>`).
struct RouteListViewModel {
  std::vector<RouteRow> routes{};
};

/// Result of updating a planetary shipping route (`route <n> ...`).
struct RouteSetResult {
  int route_num{0};
  bool active{false};
  Coordinates dest_coords{0, 0};
  std::string load{};
  std::string unload{};
  std::optional<starnum_t> dest_star{std::nullopt};
  std::optional<planetnum_t> dest_planet{std::nullopt};
};

[[nodiscard]] std::string render_route_list(const RouteListViewModel& vm);
[[nodiscard]] std::string render_json_route_list(const RouteListViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const RouteListViewModel& vm);

[[nodiscard]] std::string render_route_set_result(const RouteSetResult& vm);
[[nodiscard]] std::string
render_json_route_set_result(const RouteSetResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const RouteSetResult& vm);

/// Result of toggling automatic production reports on a planet (`autoreport
/// [<planet>]`).
struct AutoreportResult {
  starnum_t snum{0};
  planetnum_t pnum{0};
  std::string planet_name{};
  bool enabled{false};
};

[[nodiscard]] std::string render_autoreport_result(const AutoreportResult& vm);
[[nodiscard]] std::string
render_json_autoreport_result(const AutoreportResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const AutoreportResult& vm);

/// Single conqueror loot allocation row in a planetary recovery report.
struct RecoveryShareRow {
  player_t player{0};
  std::string race_name{};
  Stockpile share{};
};

/// View model for conquered planetary stockpile recovery reports.
struct RecoveryReportViewModel {
  starnum_t star_id{0};
  std::string star_name{};
  std::string planet_name{};
  planetnum_t planet_num{0};
  std::vector<RecoveryShareRow> shares{};
  Stockpile total_stolen{};
};

[[nodiscard]] std::string
render_recovery_report(const RecoveryReportViewModel& vm);
[[nodiscard]] std::string
render_json_recovery_report(const RecoveryReportViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const RecoveryReportViewModel& vm);

/// View model for per-turn automatic planetary production report telegrams.
struct PlanetTurnAutoreportViewModel {
  std::string star_name{};
  std::string planet_name{};
  bool temp_changed{false};
  temperature_t rest_temp{0};
  temperature_t current_temp{0};
  Stockpile production{};
  sector_count_t sectors_captured{0};
  int nova_stage{0};
  std::optional<Coordinates> envir_damage{std::nullopt};
  std::optional<player_t> slaved_to{std::nullopt};
};

[[nodiscard]] std::string
render_planet_turn_autoreport(const PlanetTurnAutoreportViewModel& vm);

/// View model for Stage 1 stellar nova planetary evacuation bulletins.
struct PlanetNovaBulletinViewModel {
  std::string star_name{};
  std::string planet_name{};
  bool seas_boiling{false};
};

[[nodiscard]] std::string
render_planet_nova_bulletin(const PlanetNovaBulletinViewModel& vm);

/// View model for planetary slave revolt notifications.
struct SlaveRevoltBulletinViewModel {
  std::string star_name{};
  std::string planet_name{};
  player_t former_master{0};
};

[[nodiscard]] std::string
render_slave_revolt_bulletin(const SlaveRevoltBulletinViewModel& vm);

/// Result of updating the global map centerpoint via `center <star>`.
struct CenterResult {
  starnum_t snum{0};
  std::string star_name{};
  UniverseCoordinates universe_center{0.0, 0.0};
};

[[nodiscard]] std::string render_center_result(const CenterResult& vm);
[[nodiscard]] std::string render_json_center_result(const CenterResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const CenterResult& vm);

/// Result of querying or setting the orbit map zoom scale (`zoom [<amount>]`).
struct ZoomResult {
  double zoom{1.0};
  double last_x{0.0};
  double last_y{0.0};
};

[[nodiscard]] std::string render_zoom_result(const ZoomResult& vm);
[[nodiscard]] std::string render_json_zoom_result(const ZoomResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode, const ZoomResult& vm);

/// Result of calculating Euclidean distance via `distance <from> <to>`.
struct DistanceResult {
  std::string from{};
  std::string to{};
  double distance{0.0};
};

[[nodiscard]] std::string render_distance_result(const DistanceResult& vm);
[[nodiscard]] std::string render_json_distance_result(const DistanceResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DistanceResult& vm);

/// Single star entry in a `stars [<radius>]` report.
struct StarLocationRow {
  starnum_t star_id{0};
  std::string name{};
  double x{0.0};
  double y{0.0};
  double distance{0.0};
};

/// View model for `stars [<radius>]` stellar position reports.
struct StarLocationsViewModel {
  int max_distance{999999};
  std::vector<StarLocationRow> stars{};
};

[[nodiscard]] std::string
render_star_locations(const StarLocationsViewModel& vm);
[[nodiscard]] std::string
render_json_star_locations(const StarLocationsViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const StarLocationsViewModel& vm);

/// Single planetary colony row in a `production` report.
struct ProductionRow {
  starnum_t snum{0};
  planetnum_t pnum{0};
  std::string star_name{};
  std::string planet_name{};
  char type_symbol{' '};
  bool autoreport{false};
  governor_t governor{0};
  double prod_tech{0.0};
  resource_t total_resources{0};
  resource_t prod_crystals{0};
  resource_t prod_res{0};
  resource_t prod_dest{0};
  resource_t prod_fuel{0};
  money_t prod_money{0};
  Percentage tox_thresh{0};
  double est_production{0.0};
};

/// View model for `production [<location> ...]` colony output reports.
struct ProductionReportViewModel {
  std::vector<ProductionRow> rows{};
  std::vector<std::string> bad_locations{};
};

[[nodiscard]] std::string
render_production_report(const ProductionReportViewModel& vm);
[[nodiscard]] std::string
render_json_production_report(const ProductionReportViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ProductionReportViewModel& vm);

/// View model for `technology [<investment>]` planetary tech investment.
struct TechnologyViewModel {
  bool updated{false};
  money_t investment{0};
  double tech_production{0.0};
};

[[nodiscard]] std::string render_technology(const TechnologyViewModel& vm);
[[nodiscard]] std::string render_json_technology(const TechnologyViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TechnologyViewModel& vm);

}  // namespace GB::presentation
