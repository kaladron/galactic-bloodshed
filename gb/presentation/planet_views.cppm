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

}  // namespace GB::presentation
