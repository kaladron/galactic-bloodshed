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
  std::vector<starnum_t> candidate_stars{};
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

}  // namespace GB::creator
