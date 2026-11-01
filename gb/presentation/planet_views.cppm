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

[[nodiscard]] std::string render_enslave_result(const EnslaveResult& vm);
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
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const InsurgencyResult& vm);
[[nodiscard]] std::string
render_insurgency_telegram(const InsurgencyResult& vm);
[[nodiscard]] std::string render_insurgency_news(const InsurgencyResult& vm);

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

/// Lightweight summary of a planet within a star system.
struct StarPlanetStubView {
  planetnum_t pnum{0};
  std::string name{};
  bool explored{false};
  bool inhabited{false};

  [[nodiscard]] bool
  operator==(const StarPlanetStubView&) const noexcept = default;
};

/// Explored details of a star system.
struct StarExploredView {
  double gravity{0.0};
  int temperature_class{0};
  std::optional<int> stability{std::nullopt};
  ap_t ap{0};
  governor_t governor{1};
  bool inhabited{false};
  std::vector<StarPlanetStubView> planets{};

  [[nodiscard]] bool
  operator==(const StarExploredView&) const noexcept = default;
};

/// Canonical view model for a star system.
struct StarView {
  starnum_t snum{0};
  std::string name{};
  UniverseCoordinates coordinates{0.0, 0.0};
  bool explored{false};
  std::optional<StarExploredView> details{std::nullopt};

  [[nodiscard]] bool operator==(const StarView&) const noexcept = default;
};

/// Alien presence on a surveyed planet.
struct PlanetAlienView {
  player_t player{0};
  bool at_war{false};

  [[nodiscard]] bool
  operator==(const PlanetAlienView&) const noexcept = default;
};

/// Environmental and survey data for a planet.
struct PlanetEnvironmentView {
  PlanetType type{PlanetType::EARTH};
  char type_symbol{' '};
  std::string type_name{};
  Coordinates dimensions{0, 0};
  double gravity{0.0};
  ConditionValues conditions{};
  temperature_t temp{0};
  temperature_t normal_temp{0};
  Percentage toxicity{0};
  double compatibility{0.0};
  Percentage avg_fertility{0};
  resource_t avg_resource{0};
  resource_t total_resources{0};
  std::optional<sector_count_t> crystal_sectors{std::nullopt};
  std::optional<player_t> slaved_to{std::nullopt};
  bool aliens_unknown{false};
  std::vector<PlanetAlienView> aliens{};

  [[nodiscard]] bool
  operator==(const PlanetEnvironmentView&) const noexcept = default;
};

/// Single shipping route on a colonized planet.
struct ColonyRouteView {
  int route_number{0};
  bool active{false};
  std::optional<starnum_t> dest_star{std::nullopt};
  std::optional<planetnum_t> dest_planet{std::nullopt};
  std::string dest_star_name{};
  std::string dest_planet_name{};
  Coordinates dest_coords{0, 0};
  CommodityManifest load{};
  CommodityManifest unload{};

  [[nodiscard]] bool
  operator==(const ColonyRouteView&) const noexcept = default;
};

/// Colony-specific details visible to the planet owner/governor or deity.
struct ColonyView {
  governor_t governor{1};
  bool autoreport{false};
  sector_count_t sectors_owned{0};
  population_t popn{0};
  population_t troops{0};
  population_t total_popn{0};
  population_t total_troops{0};
  population_t maxpopn{0};
  gun_count_t guns{0};
  Stockpile stockpile{};
  Stockpile last_production{};
  money_t prod_money{0};
  double prod_tech{0.0};
  double est_production{0.0};
  money_t tech_invest{0};
  Percentage tax{0};
  Percentage target_tax{0};
  Percentage mobilization{0};
  Percentage target_mobilization{0};
  std::uint32_t mob_points{0};
  std::optional<Percentage> tox_thresh{std::nullopt};
  std::vector<ColonyRouteView> routes{};

  [[nodiscard]] bool operator==(const ColonyView&) const noexcept = default;
};

/// Canonical view model for a planet.
struct PlanetView {
  starnum_t snum{0};
  planetnum_t pnum{0};
  std::string star_name{};
  std::string name{};
  SystemCoordinates system_coords{0.0, 0.0};
  bool explored{false};
  std::optional<PlanetEnvironmentView> environment{std::nullopt};
  std::optional<ColonyView> colony{std::nullopt};

  [[nodiscard]] bool operator==(const PlanetView&) const noexcept = default;
};

/// View model for querying current planetary mobilization (`mobilize`).
struct MobilizeStatusViewModel {
  PlanetView planet{};
};

/// View model for querying planetary tax rate (`tax`).
struct TaxStatusViewModel {
  PlanetView planet{};
};

/// View model for planetary shipping route inspection (`route` / `route <n>`).
struct RouteListViewModel {
  PlanetView planet{};
  std::optional<int> route_num{std::nullopt};
};

[[nodiscard]] std::string
render_mobilize_status(const MobilizeStatusViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MobilizeStatusViewModel& vm);

[[nodiscard]] std::string render_tax_status(const TaxStatusViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TaxStatusViewModel& vm);

[[nodiscard]] std::string render_route_list(const RouteListViewModel& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const RouteListViewModel& vm);

/// Landed ship on a sector.
struct SectorLandedShipView {
  shipnum_t id{0};
  char type_letter{' '};
  player_t owner{0};

  [[nodiscard]] bool
  operator==(const SectorLandedShipView&) const noexcept = default;
};

/// Detailed survey data of a sector.
struct SectorSurveyDetailsView {
  SectorType geological_type{SectorType::SEC_LAND};
  Percentage eff{0};
  Percentage fert{0};
  std::optional<Percentage> mobilization{std::nullopt};
  resource_t resource{0};
  bool crystals{false};
  population_t popn{0};
  population_t troops{0};
  population_t maxpopn{0};

  [[nodiscard]] bool
  operator==(const SectorSurveyDetailsView&) const noexcept = default;
};

/// Canonical view model for a single sector.
struct SectorView {
  Coordinates coords{0, 0};
  SectorType condition{SectorType::SEC_LAND};
  std::optional<player_t> owner{std::nullopt};
  std::vector<SectorLandedShipView> ships{};
  std::optional<SectorSurveyDetailsView> details{std::nullopt};

  [[nodiscard]] bool operator==(const SectorView&) const noexcept = default;
};

/// Canonical view model for a grid of sectors on a planet.
struct SectorGridView {
  starnum_t snum{0};
  planetnum_t pnum{0};
  Coordinates dimensions{0, 0};
  bool partial{false};
  std::vector<SectorView> sectors{};

  [[nodiscard]] bool operator==(const SectorGridView&) const noexcept = default;
};

[[nodiscard]] StarView project_star_view(EntityManager& em,
                                         const ScopeContext& viewer,
                                         starnum_t snum);

[[nodiscard]] PlanetView project_planet_view(EntityManager& em,
                                             const ScopeContext& viewer,
                                             starnum_t snum, planetnum_t pnum);

[[nodiscard]] std::optional<SectorGridView> project_sector_grid_view(
    EntityManager& em, const ScopeContext& viewer, starnum_t snum,
    planetnum_t pnum,
    const std::flat_set<Coordinates>* coords_filter = nullptr);

[[nodiscard]] std::string render_json_star_view(const StarView& vm);
[[nodiscard]] std::string render_json_planet_view(const PlanetView& vm);
[[nodiscard]] std::string
render_json_sector_grid_view(const SectorGridView& vm);

}  // namespace GB::presentation
