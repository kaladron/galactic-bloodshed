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
import std;

namespace GB::creator {

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
  double mass{1.0};
  double birthrate{1.0};
  unsigned int fighters{10};
  int iq{100};
  int iq_limit{0};
  bool metamorph{false};
  bool absorb{false};
  bool collective_iq{false};
  bool pods{false};
  double adventurism{1.0};
  unsigned int number_sexes{2};
  double metabolism{1.0};
  unsigned int fertilize{0};

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
  find_suitable_planet(PlanetType ppref, std::span<const starnum_t> star_order);

private:
  EntityManager& entity_manager_;
  JsonStore store_;
  RaceRepository races_;
};

}  // namespace GB::creator
