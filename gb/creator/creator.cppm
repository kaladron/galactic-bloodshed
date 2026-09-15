// SPDX-License-Identifier: Apache-2.0

/// \file creator.cppm
/// \brief Module interface for Galactic Bloodshed universe creation and player
/// enrollment.

module;

import strong_id;
import glaze.core;
import glaze.json;

export module gb.creator;

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import tabulate;
import std;

namespace GB::creator {

/// Returns default pre-rolled sector compatibilities and preferred sector for a
/// given home planet type.
export inline std::pair<SectorType, SectorCompatibilities>
default_sector_compatibilities_for_planet(PlanetType planet) {
  // TODO(C++26): Use std::inplace_vector when it lands in libc++ and make
  // constexpr when P3372 (constexpr containers and adaptors) lands.
  static const std::flat_map<PlanetType,
                             std::pair<SectorType, SectorCompatibilities>>
      defaults = {
          {PlanetType::EARTH,
           {SectorType::SEC_LAND,
            SectorCompatibilities{.sea = 0.5, .land = 1.0, .plated = 1.0}}},
          {PlanetType::FOREST,
           {SectorType::SEC_FOREST,
            SectorCompatibilities{.land = 0.5, .forest = 1.0, .plated = 1.0}}},
          {PlanetType::DESERT,
           {SectorType::SEC_DESERT,
            SectorCompatibilities{.mount = 0.5, .desert = 1.0, .plated = 1.0}}},
          {PlanetType::WATER,
           {SectorType::SEC_SEA,
            SectorCompatibilities{.sea = 1.0, .land = 0.5, .plated = 1.0}}},
          {PlanetType::MARS,
           {SectorType::SEC_LAND,
            SectorCompatibilities{.land = 1.0, .mount = 0.5, .plated = 1.0}}},
          {PlanetType::ICEBALL,
           {SectorType::SEC_ICE,
            SectorCompatibilities{.mount = 0.5, .ice = 1.0, .plated = 1.0}}},
          {PlanetType::GASGIANT,
           {SectorType::SEC_GAS,
            SectorCompatibilities{.gas = 1.0, .plated = 0.0}}},
      };
  if (auto it = defaults.find(planet); it != defaults.end()) {
    return it->second;
  }
  return {SectorType::SEC_LAND,
          SectorCompatibilities{.land = 1.0, .plated = 1.0}};
}

/// Forward declaration for archetype specification generator.
export struct RaceEnrollmentSpec;

/// Preset racial archetype for quick-start player enrollment.
export struct RaceArchetype {
  std::string_view name;
  PlanetType default_planet{PlanetType::EARTH};
  bool is_metamorphic{false};
  mass_t base_mass{0.5};
  birthrate_t base_birthrate{0.5};
  fighters_t base_fighters{5};
  iq_t base_iq{150};
  iq_t base_iq_limit{0};
  adventurism_t base_adventurism{0.6};
  sexes_t min_sexes{2};
  sexes_t max_sexes{4};
  metabolism_t base_metabolism{1.0};

  [[nodiscard]] mass_t sample_mass() const {
    return std::clamp(base_mass + 0.001 * int_rand(-25, 25), 0.10, 3.00);
  }
  [[nodiscard]] birthrate_t sample_birthrate() const {
    return std::clamp(base_birthrate + 0.01 * int_rand(-10, 10), 0.20, 1.00);
  }
  [[nodiscard]] fighters_t sample_fighters() const {
    int val = static_cast<int>(base_fighters) + int_rand(-1, 1);
    return static_cast<fighters_t>(std::clamp(val, 1, 20));
  }
  [[nodiscard]] iq_t sample_iq() const {
    if (is_metamorphic) {
      return 0;
    }
    int val = static_cast<int>(base_iq) + int_rand(-10, 10);
    return static_cast<iq_t>(std::clamp(val, 50, 220));
  }
  [[nodiscard]] iq_t sample_iq_limit() const {
    if (!is_metamorphic) {
      return 0;
    }
    int val = static_cast<int>(base_iq_limit) + int_rand(-10, 10);
    return static_cast<iq_t>(std::clamp(val, 50, 220));
  }
  [[nodiscard]] adventurism_t sample_adventurism() const {
    return std::clamp(base_adventurism + 0.01 * int_rand(-10, 10), 0.05, 0.99);
  }
  [[nodiscard]] sexes_t sample_sexes() const {
    int max_val =
        int_rand(static_cast<int>(min_sexes), static_cast<int>(max_sexes));
    return static_cast<sexes_t>(int_rand(static_cast<int>(min_sexes), max_val));
  }
  [[nodiscard]] metabolism_t sample_metabolism() const {
    return std::clamp(base_metabolism + 0.01 * int_rand(-15, 15), 0.10, 4.00);
  }

  /// Generates a complete RaceEnrollmentSpec with pre-rolled sector
  /// compatibilities for the selected planet type.
  [[nodiscard]] RaceEnrollmentSpec
  to_enrollment_spec(std::optional<PlanetType> planet_override = std::nullopt,
                     bool randomize = true) const;
};

export constexpr std::array<RaceArchetype, 11> race_archetypes = {{
    // 1: Metamorphic predators
    {.name = "Metamorphic Predator",
     .default_planet = PlanetType::FOREST,
     .is_metamorphic = true,
     .base_mass = 0.30,
     .base_birthrate = 0.80,
     .base_fighters = 8,
     .base_iq = 0,
     .base_iq_limit = 135,
     .base_adventurism = 0.70,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 1.00},
    // 2: Metamorphic heavyweights
    {.name = "Metamorphic Heavyweight",
     .default_planet = PlanetType::DESERT,
     .is_metamorphic = true,
     .base_mass = 1.60,
     .base_birthrate = 0.70,
     .base_fighters = 10,
     .base_iq = 0,
     .base_iq_limit = 130,
     .base_adventurism = 0.60,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 1.15},
    // 3: Metamorphic colossi
    {.name = "Metamorphic Colossus",
     .default_planet = PlanetType::MARS,
     .is_metamorphic = true,
     .base_mass = 2.50,
     .base_birthrate = 0.65,
     .base_fighters = 12,
     .base_iq = 0,
     .base_iq_limit = 125,
     .base_adventurism = 0.55,
     .min_sexes = 1,
     .max_sexes = 1,
     .base_metabolism = 1.20},
    // 4: High intelligence, low combat
    {.name = "Cerebral Researcher",
     .default_planet = PlanetType::EARTH,
     .is_metamorphic = false,
     .base_mass = 0.50,
     .base_birthrate = 0.60,
     .base_fighters = 2,
     .base_iq = 190,
     .base_iq_limit = 0,
     .base_adventurism = 0.50,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.00},
    // 5
    {.name = "High IQ Scholar",
     .default_planet = PlanetType::WATER,
     .is_metamorphic = false,
     .base_mass = 0.65,
     .base_birthrate = 0.60,
     .base_fighters = 4,
     .base_iq = 185,
     .base_iq_limit = 0,
     .base_adventurism = 0.50,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.05},
    // 6
    {.name = "Progressive Technocrat",
     .default_planet = PlanetType::EARTH,
     .is_metamorphic = false,
     .base_mass = 0.75,
     .base_birthrate = 0.65,
     .base_fighters = 6,
     .base_iq = 175,
     .base_iq_limit = 0,
     .base_adventurism = 0.65,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.10},
    // 7
    {.name = "Balanced Expansionist",
     .default_planet = PlanetType::EARTH,
     .is_metamorphic = false,
     .base_mass = 0.85,
     .base_birthrate = 0.70,
     .base_fighters = 7,
     .base_iq = 160,
     .base_iq_limit = 0,
     .base_adventurism = 0.70,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.20},
    // 8
    {.name = "Adaptive Explorer",
     .default_planet = PlanetType::ICEBALL,
     .is_metamorphic = false,
     .base_mass = 0.75,
     .base_birthrate = 0.70,
     .base_fighters = 7,
     .base_iq = 150,
     .base_iq_limit = 0,
     .base_adventurism = 0.85,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.40},
    // 9
    {.name = "Aggressive Colonizer",
     .default_planet = PlanetType::FOREST,
     .is_metamorphic = false,
     .base_mass = 0.85,
     .base_birthrate = 0.85,
     .base_fighters = 8,
     .base_iq = 145,
     .base_iq_limit = 0,
     .base_adventurism = 0.75,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.15},
    // 10: Balanced military
    {.name = "Militaristic Legionnaire",
     .default_planet = PlanetType::MARS,
     .is_metamorphic = false,
     .base_mass = 1.20,
     .base_birthrate = 0.70,
     .base_fighters = 12,
     .base_iq = 140,
     .base_iq_limit = 0,
     .base_adventurism = 0.65,
     .min_sexes = 2,
     .max_sexes = 4,
     .base_metabolism = 1.20},
    // 11: Jovian gas giant dweller
    {.name = "Jovian Gas Floater",
     .default_planet = PlanetType::GASGIANT,
     .is_metamorphic = false,
     .base_mass = 0.40,
     .base_birthrate = 0.60,
     .base_fighters = 5,
     .base_iq = 165,
     .base_iq_limit = 0,
     .base_adventurism = 0.70,
     .min_sexes = 2,
     .max_sexes = 2,
     .base_metabolism = 1.00},
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
  birthrate_t birthrate{0.6};
  fighters_t fighters{4};
  iq_t iq{150};
  iq_t iq_limit{0};
  bool metamorph{false};
  bool absorb{false};
  bool collective_iq{false};
  bool pods{false};
  adventurism_t adventurism{0.4};
  sexes_t number_sexes{2};
  metabolism_t metabolism{1.0};
  fertilize_t fertilize{0};

  // Sector compatibility preferences (0.0 to 1.0 per SectorType)
  SectorCompatibilities sector_compatibilities{};
  std::optional<SectorType> likesbest{std::nullopt};
};

}  // namespace GB::creator

export namespace glz {

template <>
struct meta<SectorCompatibilities> {
  using T = SectorCompatibilities;
  static constexpr auto value =
      object("sea", &T::sea, "land", &T::land, "mount", &T::mount, "gas",
             &T::gas, "ice", &T::ice, "forest", &T::forest, "desert",
             &T::desert, "plated", &T::plated, "wasted", &T::wasted);
};

template <>
struct meta<GB::creator::RaceEnrollmentSpec> {
  using T = GB::creator::RaceEnrollmentSpec;
  static constexpr auto value = object(
      "name", &T::name, "password", &T::password, "governor_password",
      &T::governor_password, "address", &T::address, "home_planet_type",
      &T::home_planet_type, "preferred_sector", &T::preferred_sector,
      "capital_coords", &T::capital_coords, "target_planet", &T::target_planet,
      "candidate_stars", &T::candidate_stars, "is_god", &T::is_god, "is_guest",
      &T::is_guest, "mass", &T::mass, "birthrate", &T::birthrate, "fighters",
      &T::fighters, "iq", &T::iq, "iq_limit", &T::iq_limit, "metamorph",
      &T::metamorph, "absorb", &T::absorb, "collective_iq", &T::collective_iq,
      "pods", &T::pods, "adventurism", &T::adventurism, "number_sexes",
      &T::number_sexes, "metabolism", &T::metabolism, "fertilize",
      &T::fertilize, "sector_compatibilities", &T::sector_compatibilities,
      "likesbest", &T::likesbest);
};

}  // namespace glz

namespace GB::creator {

/// Default JSON filename for saving and loading race specifications.
export constexpr std::string_view DEFAULT_RACEGEN_FILENAME = "racegen.json";

/// Serializes a RaceEnrollmentSpec to a JSON file using Glaze.
export std::expected<void, std::string>
save_race_spec(const RaceEnrollmentSpec& spec,
               const std::filesystem::path& path = DEFAULT_RACEGEN_FILENAME);

/// Deserializes a RaceEnrollmentSpec from a JSON file using Glaze.
export std::expected<RaceEnrollmentSpec, std::string>
load_race_spec(const std::filesystem::path& path = DEFAULT_RACEGEN_FILENAME);

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
  explicit EnrollmentService(EntityManager& em);

  /// Enrolls a new player empire using the provided specification.
  EnrollmentResult enroll_player(const RaceEnrollmentSpec& spec);

  /// Discovers a vacant candidate planet of the requested type in an
  /// uninhabited multi-planet system.
  std::optional<std::pair<starnum_t, planetnum_t>>
  find_suitable_planet(PlanetType ppref,
                       std::span<const starnum_t> star_order = {});

private:
  EntityManager& entity_manager_;
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
  SectorCompatibilities sector_costs{.plated = 0.0};
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

/// Interactive session for player race generation and customization.
export class RacegenSession {
public:
  explicit RacegenSession(std::istream& in = std::cin,
                          std::ostream& out = std::cout,
                          EnrollmentService* enrollment_service = nullptr);

  /// Runs the interactive command loop until 'quit' or EOF.
  void run();

  /// Executes a single command line. Returns true if session should continue,
  /// false if 'quit' was requested.
  bool execute_command(std::string_view line);

  /// Accessors for state inspection.
  [[nodiscard]] const RaceEnrollmentSpec& spec() const noexcept {
    return spec_;
  }
  [[nodiscard]] RaceEnrollmentSpec& mutable_spec() noexcept {
    return spec_;
  }
  [[nodiscard]] const RaceCostBreakdown& cost() const noexcept {
    return cost_;
  }
  [[nodiscard]] bool should_quit() const noexcept {
    return quit_requested_;
  }

  /// Saves current race specification to a JSON file.
  bool save_to_file(const std::filesystem::path& path);

  /// Loads race specification from a JSON file.
  bool load_from_file(const std::filesystem::path& path);

  /// Attempts to enroll the player with the current specification.
  EnrollmentResult enroll();

  /// Prints formatted race specification and cost breakdown to output.
  void print_race();

  /// Prints help text for available commands or a specific topic.
  void print_help(std::string_view topic = "");

  /// Modifies a field in the race specification. Returns true on success,
  /// or false (with error output) on invalid field/value/bounds.
  bool modify_field(std::string_view field, std::string_view value);

private:
  std::istream& in_;
  std::ostream& out_;
  EnrollmentService* enrollment_service_{nullptr};
  RacegenEngine engine_;
  RaceEnrollmentSpec spec_;
  RaceCostBreakdown cost_;
  bool quit_requested_{false};

  struct CommandDescriptor {
    std::string_view name;
    std::string_view syntax;
    std::string_view description;
    bool (RacegenSession::*handler)(std::string_view args);
  };
  static const std::array<CommandDescriptor, 7>& commands();

  void update_cost();
  bool do_modify(std::string_view args);
  bool do_print(std::string_view args);
  bool do_save(std::string_view args);
  bool do_load(std::string_view args);
  bool do_enroll(std::string_view args);
  bool do_help(std::string_view args);
  bool do_quit(std::string_view args);
};

}  // namespace GB::creator
