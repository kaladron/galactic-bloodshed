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
// Created fresh at the start of each turn.
export struct TurnStats {
  // Per-star population counts for each player (keyed by 1-based starnum_t)
  std::unordered_map<starnum_t, PlayerVector<population_t, MAXPLAYERS>>
      starpopns{};

  // Per-star ship counts for each player (keyed by 1-based starnum_t)
  std::unordered_map<starnum_t, PlayerVector<ship_count_t, MAXPLAYERS>>
      starnumships{};

  // --- Planetary Simulation Tracking ---

  /// \brief Checks that star and planet numbers are valid 1-based IDs (>= 1),
  /// throwing std::out_of_range if not.
  static constexpr void check_planet_bounds(starnum_t snum, planetnum_t pnum) {
    if (snum.value < 1) {
      throw std::out_of_range(
          std::format("Star index {} out of range (must be >= 1)", snum.value));
    }
    if (pnum.value < 1) {
      throw std::out_of_range(std::format(
          "Planet index {} out of range (must be >= 1)", pnum.value));
    }
  }

  /// \brief Returns the temperature adjustment for the specified planet.
  [[nodiscard]] temp_delta_t temp_add(starnum_t snum, planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    auto it = planet_turn_info_.find({snum, pnum});
    return it != planet_turn_info_.end() ? it->second.temp_add : 0;
  }

  /// \brief Directly sets the temperature adjustment for the specified planet.
  void set_temp_add(starnum_t snum, planetnum_t pnum, temp_delta_t temp) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[{snum, pnum}].temp_add = temp;
  }

  /// \brief Adds a delta to the temperature adjustment for the specified
  /// planet.
  void add_temp(starnum_t snum, planetnum_t pnum, temp_delta_t delta) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[{snum, pnum}].temp_add += delta;
  }

  /// \brief Returns whether slave revolts are intimidated on the specified
  /// planet.
  [[nodiscard]] bool is_intimidated(starnum_t snum, planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    auto it = planet_turn_info_.find({snum, pnum});
    return it != planet_turn_info_.end() ? it->second.intimidated : false;
  }

  /// \brief Sets whether slave revolts are intimidated on the specified planet.
  void set_intimidated(starnum_t snum, planetnum_t pnum,
                       bool intimidated = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[{snum, pnum}].intimidated = intimidated;
  }

  /// \brief Returns whether any race inhabits or explored this planet this
  /// turn.
  [[nodiscard]] bool is_inhabited(starnum_t snum, planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    auto it = planet_turn_info_.find({snum, pnum});
    return it != planet_turn_info_.end() ? it->second.inhabited : false;
  }

  /// \brief Marks whether any race inhabits or explored this planet this turn.
  void mark_inhabited(starnum_t snum, planetnum_t pnum, bool inhabited = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[{snum, pnum}].inhabited = inhabited;
  }

  /// \brief Returns whether an alien colony has spawned on this planet this
  /// turn.
  [[nodiscard]] bool has_alien_colony(starnum_t snum, planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    auto it = planet_turn_info_.find({snum, pnum});
    return it != planet_turn_info_.end() ? it->second.alien_colony : false;
  }

  /// \brief Sets whether an alien colony has spawned on this planet this turn.
  void set_alien_colony(starnum_t snum, planetnum_t pnum, bool spawned = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[{snum, pnum}].alien_colony = spawned;
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

  // Non-copyable to prevent accidental copies
  TurnStats(const TurnStats&) = delete;
  TurnStats& operator=(const TurnStats&) = delete;

  // Default constructor value-initializes all members
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

  std::map<std::pair<starnum_t, planetnum_t>, PlanetTurnInfo>
      planet_turn_info_{};
};
