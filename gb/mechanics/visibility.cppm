// SPDX-License-Identifier: Apache-2.0

/// \file visibility.cppm
/// \brief Planetary surface map rendering and sector character visibility.

export module gb.mechanics:visibility;

import gb.entities;
import gb.services;
import std;

export struct PlanetMapCell {
  Coordinates coords{0, 0};
  player_t owner{0};
  char glyph{' '};
  bool inverse{false};

  [[nodiscard]] bool operator==(const PlanetMapCell&) const noexcept = default;
};

export struct PlanetAlienPresence {
  player_t player{0};
  bool at_war{false};

  [[nodiscard]] bool
  operator==(const PlanetAlienPresence&) const noexcept = default;
};

export struct PlanetMapViewModel {
  std::string planet_name;
  Coordinates dimensions{0, 0};
  std::vector<PlanetMapCell> sectors;
  std::string planet_type_name;
  bool is_metamorph{false};
  sector_count_t sectors_owned{0};
  bool aliens_unknown{false};
  std::vector<PlanetAlienPresence> aliens;
  std::uint32_t guns{0};
  std::uint32_t mob_points{0};
  Percentage comread{0};
  Percentage mob_set{0};
  double compatibility{0.0};
  Percentage toxicity{0};
  resource_t resource_stockpile{0};
  resource_t fuel_stockpile{0};
  resource_t destruct_cap{0};
  population_t player_popn{0};
  population_t total_popn{0};
  population_t effective_maxpopn{0};
  resource_t crystals{0};
  population_t player_troops{0};
  population_t total_troops{0};
  resource_t total_resources{0};
  Percentage tax{0};
  Percentage newtax{0};
  double est_production{0.0};
  std::optional<player_t> slaved_to{std::nullopt};
  bool primary_unstable{false};

  [[nodiscard]] bool
  operator==(const PlanetMapViewModel&) const noexcept = default;
};

export char desshow(player_t, governor_t, const Race&, const Sector&);
export PlanetMapViewModel build_planet_map(EntityManager& em, starnum_t snum,
                                           planetnum_t pnum, const Planet& p,
                                           player_t playernum,
                                           governor_t governor,
                                           const Race& race);
