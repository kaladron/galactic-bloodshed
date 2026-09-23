// SPDX-License-Identifier: Apache-2.0

/// \file turnstats.cppm
/// \brief Module interface partition for TurnStats turn-scoped statistics
/// accumulator.

module;

import std;

export module gb.turn:turnstats;

import gb.entities;

// TurnStats: Encapsulates per-turn accumulating statistics.
// Passed through doplanet() and doship() to replace global array usage.
// Created fresh at the start of each turn; value-initialization zeros all
// arrays.
export struct TurnStats {
  // Per-star population counts for each player (1-indexed: 1..NUMSTARS)
  std::array<PlayerVector<population_t, MAXPLAYERS>, NUMSTARS + 1> starpopns{};

  // Per-star ship counts for each player (1-indexed: 1..NUMSTARS)
  std::array<PlayerVector<ship_count_t, MAXPLAYERS>, NUMSTARS + 1>
      starnumships{};

  // --- Planetary Simulation Tracking ---

  /// \brief Checks that star and planet numbers are within 1-based bounds,
  /// throwing std::out_of_range if not.
  static constexpr void check_planet_bounds(starnum_t snum, planetnum_t pnum) {
    if (snum.value < 1 || snum.value > NUMSTARS) {
      throw std::out_of_range(std::format("Star index {} out of range (1..{})",
                                          snum.value, NUMSTARS));
    }
    if (pnum.value < 1 || pnum.value > MAXPLANETS) {
      throw std::out_of_range(std::format(
          "Planet index {} out of range (1..{})", pnum.value, MAXPLANETS));
    }
  }

  /// \brief Returns the temperature adjustment for the specified planet.
  [[nodiscard]] constexpr temp_delta_t temp_add(starnum_t snum,
                                                planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value - 1][pnum.value - 1].temp_add;
  }

  /// \brief Directly sets the temperature adjustment for the specified planet.
  constexpr void set_temp_add(starnum_t snum, planetnum_t pnum,
                              temp_delta_t temp) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value - 1][pnum.value - 1].temp_add = temp;
  }

  /// \brief Adds a delta to the temperature adjustment for the specified
  /// planet.
  constexpr void add_temp(starnum_t snum, planetnum_t pnum,
                          temp_delta_t delta) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value - 1][pnum.value - 1].temp_add += delta;
  }

  /// \brief Returns whether slave revolts are intimidated on the specified
  /// planet.
  [[nodiscard]] constexpr bool is_intimidated(starnum_t snum,
                                              planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value - 1][pnum.value - 1].intimidated;
  }

  /// \brief Sets whether slave revolts are intimidated on the specified planet.
  constexpr void set_intimidated(starnum_t snum, planetnum_t pnum,
                                 bool intimidated = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value - 1][pnum.value - 1].intimidated = intimidated;
  }

  /// \brief Returns whether any race inhabits or explored this planet this
  /// turn.
  [[nodiscard]] constexpr bool is_inhabited(starnum_t snum,
                                            planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value - 1][pnum.value - 1].inhabited;
  }

  /// \brief Marks whether any race inhabits or explored this planet this turn.
  constexpr void mark_inhabited(starnum_t snum, planetnum_t pnum,
                                bool inhabited = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value - 1][pnum.value - 1].inhabited = inhabited;
  }

  /// \brief Returns whether an alien colony has spawned on this planet this
  /// turn.
  [[nodiscard]] constexpr bool has_alien_colony(starnum_t snum,
                                                planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value - 1][pnum.value - 1].alien_colony;
  }

  /// \brief Sets whether an alien colony has spawned on this planet this turn.
  constexpr void set_alien_colony(starnum_t snum, planetnum_t pnum,
                                  bool spawned = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value - 1][pnum.value - 1].alien_colony = spawned;
  }

  // Power statistics for each player
  PlayerVector<power, MAXPLAYERS> Power;

  // Production statistics per player (per-planet accumulators)
  PlayerVector<resource_t, MAXPLAYERS> prod_res;
  PlayerVector<resource_t, MAXPLAYERS> prod_fuel;
  PlayerVector<resource_t, MAXPLAYERS> prod_destruct;
  PlayerVector<resource_t, MAXPLAYERS> prod_crystals;

  /// \brief Accumulates a sector's produced stockpile (resources, destruct,
  /// fuel, crystals) into the owning player's per-turn production totals.
  void record_production(player_t owner, const Stockpile& produced) noexcept {
    prod_res[owner] += produced.resources;
    prod_destruct[owner] += produced.destruct;
    prod_fuel[owner] += produced.fuel;
    prod_crystals[owner] += produced.crystals;
  }

  // Per-planet total sector mobilization points accumulated per player
  PlayerVector<std::uint32_t, MAXPLAYERS> total_mob_points;

  // Per-planet count of newly captured/spread sectors
  sector_count_t tot_captured{};

  // Compatibility values per player (computed at planet start)
  PlayerVector<double, MAXPLAYERS> Compat;

  // Claims flag (set if any sector ownership changes)
  bool Claims{};

  // VN brain state (VN AI state per turn)
  Vnbrain VN_brain{};

  // Non-copyable to prevent accidental copies of large arrays
  TurnStats(const TurnStats&) = delete;
  TurnStats& operator=(const TurnStats&) = delete;

  // Default constructor value-initializes (zeros) all arrays
  TurnStats() = default;

  // Movable for container usage if needed
  TurnStats(TurnStats&&) = default;
  TurnStats& operator=(TurnStats&&) = default;

private:
  /// \brief Temporary per-planet simulation state tracking across turn update
  /// passes.
  struct PlanetTurnInfo {
    temp_delta_t temp_add{
        0};  ///< Thermal adjustment applied to planet temperature
    bool alien_colony{
        false};  ///< Whether a new alien Thing colony spawned on this planet
    bool inhabited{false};  ///< Whether any race inhabits or explored this
                            ///< planet this turn
    bool intimidated{
        false};  ///< Whether an assault platform is suppressing slave revolts
  };

  std::array<std::array<PlanetTurnInfo, MAXPLANETS>, NUMSTARS>
      planet_turn_info_{};
};
