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

}  // namespace GB::presentation
