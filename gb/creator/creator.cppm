// SPDX-License-Identifier: Apache-2.0

/// \file creator.cppm
/// \brief Module interface for Galactic Bloodshed universe creation and player
/// enrollment.

module;

export module gb.creator;

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import tabulate;
import std;

namespace GB::creator {

/// Preset racial archetype for quick-start player enrollment.
export struct RaceArchetype {
  std::string_view name;
  bool is_metamorphic{false};
  mass_t base_mass{0.125};
  birthrate_t base_birthrate{0.5};
  fighters_t base_fighters{5};
  iq_t base_iq{150};
  adventurism_t base_adventurism{0.7};
  sexes_t min_sexes{2};
  sexes_t max_sexes{4};
  metabolism_t base_metabolism{1.5};

  [[nodiscard]] mass_t sample_mass() const {
    return base_mass + 0.001 * int_rand(-25, 25);
  }
  [[nodiscard]] birthrate_t sample_birthrate() const {
    return base_birthrate + 0.01 * int_rand(-10, 10);
  }
  [[nodiscard]] fighters_t sample_fighters() const {
    int val = static_cast<int>(base_fighters) + int_rand(-1, 1);
    return static_cast<fighters_t>(std::max(0, val));
  }
  [[nodiscard]] iq_t sample_iq() const {
    if (is_metamorphic) {
      return 0;
    }
    return base_iq + int_rand(-10, 10);
  }
  [[nodiscard]] adventurism_t sample_adventurism() const {
    return base_adventurism + 0.01 * int_rand(-10, 10);
  }
  [[nodiscard]] sexes_t sample_sexes() const {
    int max_val =
        int_rand(static_cast<int>(min_sexes), static_cast<int>(max_sexes));
    return static_cast<sexes_t>(int_rand(static_cast<int>(min_sexes), max_val));
  }
  [[nodiscard]] metabolism_t sample_metabolism() const {
    return base_metabolism + 0.01 * int_rand(-15, 15);
  }
};

export constexpr std::array<RaceArchetype, 10> race_archetypes = {{
    // 1: Metamorphic predators
    {.name = "Metamorphic Predator",
     .is_metamorphic = true,
     .base_mass = 0.1,
     .base_birthrate = 0.9,
     .base_fighters = 9,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 3.0},
    // 2: Metamorphic heavyweights
    {.name = "Metamorphic Heavyweight",
     .is_metamorphic = true,
     .base_mass = 0.15,
     .base_birthrate = 0.85,
     .base_fighters = 10,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 2.7},
    // 3: Metamorphic colossi
    {.name = "Metamorphic Colossus",
     .is_metamorphic = true,
     .base_mass = 0.2,
     .base_birthrate = 0.8,
     .base_fighters = 11,
     .base_iq = 0,
     .base_adventurism = 0.89,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 2.4},
    // 4: High intelligence, low combat
    {.name = "Cerebral Researcher",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.5,
     .base_fighters = 2,
     .base_iq = 190,
     .base_adventurism = 0.6,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.0},
    // 5
    {.name = "High IQ Scholar",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.55,
     .base_fighters = 3,
     .base_iq = 180,
     .base_adventurism = 0.65,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.15},
    // 6
    {.name = "Progressive Technocrat",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.6,
     .base_fighters = 4,
     .base_iq = 170,
     .base_adventurism = 0.7,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.30},
    // 7
    {.name = "Balanced Expansionist",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.65,
     .base_fighters = 5,
     .base_iq = 160,
     .base_adventurism = 0.7,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.45},
    // 8
    {.name = "Adaptive Explorer",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.7,
     .base_fighters = 6,
     .base_iq = 150,
     .base_adventurism = 0.75,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.6},
    // 9
    {.name = "Aggressive Colonizer",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.75,
     .base_fighters = 7,
     .base_iq = 140,
     .base_adventurism = 0.75,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.75},
    // 10: Balanced military
    {.name = "Militaristic Legionnaire",
     .is_metamorphic = false,
     .base_mass = 0.125,
     .base_birthrate = 0.8,
     .base_fighters = 8,
     .base_iq = 130,
     .base_adventurism = 0.8,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.9},
}};

/// Builds a formatted tabulate::Table summarizing all preset racial archetypes.
export tabulate::Table create_archetypes_table();

/// Specification for enrolling a new player empire into the game.
export struct RaceEnrollmentSpec {
  std::string name;
  std::string password;
  std::string governor_password{"0"};
  std::string address;
  PlanetType home_planet_type{PlanetType::EARTH};
  std::optional<SectorType> preferred_sector{std::nullopt};
  std::optional<Coordinates> capital_coords{std::nullopt};
  std::optional<std::pair<starnum_t, planetnum_t>> target_planet{std::nullopt};
  std::vector<starnum_t> candidate_stars;
  bool is_god{false};
  bool is_guest{false};

  // Biological & racial attributes
  mass_t mass{1.0};
  birthrate_t birthrate{1.0};
  fighters_t fighters{10};
  iq_t iq{100};
  iq_t iq_limit{0};
  bool metamorph{false};
  bool absorb{false};
  bool collective_iq{false};
  bool pods{false};
  adventurism_t adventurism{1.0};
  sexes_t number_sexes{2};
  metabolism_t metabolism{1.0};
  fertilize_t fertilize{0};

  // Sector compatibility preferences (0.0 to 1.0 per SectorType)
  std::array<double, SectorType::SEC_WASTED + 1> sector_compatibilities{};
  std::optional<SectorType> likesbest{std::nullopt};
};

/// Result of an enrollment attempt.
export struct EnrollmentResult {
  bool success{false};
  player_t player_num{0};
  starnum_t star{0};
  planetnum_t pnum{0};
  Coordinates capital_coords{0, 0};
  shipnum_t gov_ship{0};
  std::string message;
};

/// Domain service coordinating player empire enrollment.
export class EnrollmentService {
public:
  EnrollmentService(EntityManager& em, Database& db);

  /// Enrolls a new player empire using the provided specification.
  EnrollmentResult enroll_player(const RaceEnrollmentSpec& spec);

  /// Discovers a vacant candidate planet of the requested type in an
  /// uninhabited multi-planet system.
  std::optional<std::pair<starnum_t, planetnum_t>>
  find_suitable_planet(PlanetType ppref,
                       std::span<const starnum_t> star_order = {});

private:
  EntityManager& entity_manager_;
  JsonStore store_;
  RaceRepository races_;
};

/// Baseline cosmic temperature floor in Celsius (~4 Kelvin, near cosmic
/// background).
export constexpr int BASELINE_SPACE_TEMP_C = -269;

/// Stellar luminosity scaling coefficient used for orbital radiant flux.
export constexpr double STELLAR_LUMINOSITY_SCALE = 1315.0;

/// Core orbital scale radius offset in orbital distance units.
export constexpr double ORBITAL_SCALE_RADIUS = 40.0;

/// Temperature calculation formula based on orbital distance and star
/// spectral temperature index.
///
/// Equilibrium surface temperature in Celsius:
///   T = T_baseline + (T_star * L_scale * R_core) / (R_core + dist)
export constexpr int calculate_temperature(double dist, int stemp) noexcept {
  return BASELINE_SPACE_TEMP_C +
         static_cast<int>(stemp * STELLAR_LUMINOSITY_SCALE *
                          ORBITAL_SCALE_RADIUS / (ORBITAL_SCALE_RADIUS + dist));
}

/// Generates an individual planet for a star system with procedural terrain.
export Planet makeplanet(double dist, short stemp, PlanetType type,
                         starnum_t star_id, planetnum_t planet_order,
                         std::optional<SectorMap>& out_smap);

/// Configuration parameters for procedural universe creation.
export struct UniverseConfig {
  starnum_t num_stars{128};
  int min_planets{1};
  int max_planets{10};
  int planetless_chance_percent{0};
  bool auto_name_stars{true};
  bool auto_name_planets{true};
  bool print_star_info{false};
  bool print_planet_info{false};
  std::string star_names_file{PKGDATADIR "star.list"};
  std::string planet_names_file{PKGDATADIR "planet.list"};
  std::string exam_file{PKGDATADIR "exam.dat"};
};

/// Summary of a generated universe.
export struct UniverseGenerationResult {
  starnum_t num_stars{0};
  int planet_count{0};
  int total_resources{0};
  std::array<int, PlanetType::DESERT + 1> planets_by_type{};
};

/// Procedural engine that generates stars, planets, sectormaps, and universe
/// metadata.
export class UniverseGenerator {
public:
  explicit UniverseGenerator(UniverseConfig config = {});

  /// Sets custom star name list (useful for tests or overriding files).
  void set_star_names(std::vector<std::string> names);

  /// Sets custom planet name list (useful for tests or overriding files).
  void set_planet_names(std::vector<std::string> names);

  /// Generates the complete universe into the database.
  UniverseGenerationResult generate(Database& db);

private:
  UniverseConfig config_;
  std::array<std::array<bool, 100>, 100> star_grid_occupancy_{};
  std::vector<std::string> star_names_;
  std::vector<std::size_t> star_indices_;
  std::size_t star_name_cursor_{0};

  std::vector<std::string> planet_names_;
  std::vector<std::size_t> planet_indices_;
  std::size_t planet_name_cursor_{0};

  void load_name_lists();
  std::string next_star_name(starnum_t snum);
  std::string next_planet_name(planetnum_t pnum);
  void place_star(star_struct& star);
  Star make_star_system(Database& db, starnum_t snum,
                        UniverseGenerationResult& result);
};

export constexpr std::size_t num_race_attributes = 11;

/// Itemized cost breakdown resulting from race point calculations.
export struct RaceCostBreakdown {
  std::array<double, num_race_attributes> attribute_costs{};
  std::array<double, SectorType::SEC_WASTED + 1> sector_costs{};
  int planet_cost{0};
  int race_type_cost{0};
  int sector_count_cost{0};
  int total_cost{0};
  int points_remaining{STARTING_POINTS};

  [[nodiscard]] double adventurism() const noexcept {
    return attribute_costs[0];
  }
  [[nodiscard]] double absorb() const noexcept {
    return attribute_costs[1];
  }
  [[nodiscard]] double birthrate() const noexcept {
    return attribute_costs[2];
  }
  [[nodiscard]] double collective_iq() const noexcept {
    return attribute_costs[3];
  }
  [[nodiscard]] double fertilize() const noexcept {
    return attribute_costs[4];
  }
  [[nodiscard]] double iq() const noexcept {
    return attribute_costs[5];
  }
  [[nodiscard]] double fight() const noexcept {
    return attribute_costs[6];
  }
  [[nodiscard]] double pods() const noexcept {
    return attribute_costs[7];
  }
  [[nodiscard]] double mass() const noexcept {
    return attribute_costs[8];
  }
  [[nodiscard]] double sexes() const noexcept {
    return attribute_costs[9];
  }
  [[nodiscard]] double metabolism() const noexcept {
    return attribute_costs[10];
  }
};

/// Core calculation and validation engine for racegen.
export class RacegenEngine {
public:
  RacegenEngine();

  /// Creates a clean default enrollment specification.
  [[nodiscard]] RaceEnrollmentSpec
  create_default_spec(bool metamorph = false) const noexcept;

  /// Computes the complete itemized cost and remaining points for an enrollment
  /// spec.
  [[nodiscard]] RaceCostBreakdown
  calculate_cost(const RaceEnrollmentSpec& spec) const noexcept;

  /// Validates an enrollment spec against game invariants and budget
  /// constraints.
  [[nodiscard]] std::vector<std::string>
  validate(const RaceEnrollmentSpec& spec, bool is_player = true,
           bool rigorous = false) const;

  /// Returns whether an enrollment spec satisfies all validation invariants.
  [[nodiscard]] bool is_valid(const RaceEnrollmentSpec& spec,
                              bool is_player = true,
                              bool rigorous = false) const {
    return validate(spec, is_player, rigorous).empty();
  }

private:
  struct AttributeParam {
    double e_factor{0.0};
    double e_fudge{0.0};
    double e_hinge{0.0};
    double l_factor{0.0};
    double l_fudge{0.0};
    double minimum{0.0};
    double init{0.0};
    double maximum{0.0};
    int is_integral{0};  // 0 = float, 1 = int, 2 = bool
  };

  std::array<AttributeParam, num_race_attributes> base_attr_{};
  std::array<std::array<double, num_race_attributes>, num_race_attributes>
      normal_cov_{};
  std::array<std::array<double, num_race_attributes>, num_race_attributes>
      morph_cov_{};
};

}  // namespace GB::creator
