// SPDX-License-Identifier: Apache-2.0

/// \file gblib-turnstats.cppm
/// \brief Module interface partition for TurnStats turn-scoped statistics
/// accumulator.

module;

import std;

export module gblib:turnstats;

import :race;
import :tweakables;
import :types;

// TurnStats: Encapsulates per-turn accumulating statistics.
// Passed through doplanet() and doship() to replace global array usage.
// Created fresh at the start of each turn; value-initialization zeros all
// arrays.
export struct TurnStats {
  // Per-star population counts for each player
  std::array<PlayerVector<unsigned long, MAXPLAYERS>, NUMSTARS> starpopns{};

  // Per-star ship counts for each player
  std::array<PlayerVector<unsigned short, MAXPLAYERS>, NUMSTARS> starnumships{};

  // Global ship counts per player (for Sdata)
  PlayerVector<unsigned short, MAXPLAYERS> Sdatanumships;

  // Global population counts per player (for Sdata)
  PlayerVector<unsigned long, MAXPLAYERS> Sdatapopns;

  // --- Planetary Simulation Tracking ---

  /// \brief Checks that star and planet numbers are within bounds, throwing
  /// std::out_of_range if not.
  static constexpr void check_planet_bounds(starnum_t snum, planetnum_t pnum) {
    if (snum.value >= NUMSTARS) {
      throw std::out_of_range(std::format("Star index {} out of range (0..{})",
                                          snum.value, NUMSTARS - 1));
    }
    if (pnum.value >= MAXPLANETS) {
      throw std::out_of_range(std::format(
          "Planet index {} out of range (0..{})", pnum.value, MAXPLANETS - 1));
    }
  }

  /// \brief Returns the temperature adjustment for the specified planet.
  [[nodiscard]] constexpr temp_delta_t temp_add(starnum_t snum,
                                                planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value][pnum.value].temp_add;
  }

  /// \brief Directly sets the temperature adjustment for the specified planet.
  constexpr void set_temp_add(starnum_t snum, planetnum_t pnum,
                              temp_delta_t temp) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value][pnum.value].temp_add = temp;
  }

  /// \brief Adds a delta to the temperature adjustment for the specified
  /// planet.
  constexpr void add_temp(starnum_t snum, planetnum_t pnum,
                          temp_delta_t delta) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value][pnum.value].temp_add += delta;
  }

  /// \brief Returns whether slave revolts are intimidated on the specified
  /// planet.
  [[nodiscard]] constexpr bool is_intimidated(starnum_t snum,
                                              planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value][pnum.value].intimidated;
  }

  /// \brief Sets whether slave revolts are intimidated on the specified planet.
  constexpr void set_intimidated(starnum_t snum, planetnum_t pnum,
                                 bool intimidated = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value][pnum.value].intimidated = intimidated;
  }

  /// \brief Returns whether any race inhabits or explored this planet this
  /// turn.
  [[nodiscard]] constexpr bool is_inhabited(starnum_t snum,
                                            planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value][pnum.value].inhabited;
  }

  /// \brief Marks whether any race inhabits or explored this planet this turn.
  constexpr void mark_inhabited(starnum_t snum, planetnum_t pnum,
                                bool inhabited = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value][pnum.value].inhabited = inhabited;
  }

  /// \brief Returns whether an alien colony has spawned on this planet this
  /// turn.
  [[nodiscard]] constexpr bool has_alien_colony(starnum_t snum,
                                                planetnum_t pnum) const {
    check_planet_bounds(snum, pnum);
    return planet_turn_info_[snum.value][pnum.value].alien_colony;
  }

  /// \brief Sets whether an alien colony has spawned on this planet this turn.
  constexpr void set_alien_colony(starnum_t snum, planetnum_t pnum,
                                  bool spawned = true) {
    check_planet_bounds(snum, pnum);
    planet_turn_info_[snum.value][pnum.value].alien_colony = spawned;
  }

  // Stars inhabited bitmap (one per star)
  std::array<unsigned long, NUMSTARS> StarsInhab{};

  // Stars explored bitmap (one per star)
  std::array<unsigned long, NUMSTARS> StarsExpl{};

  // Power statistics for each player
  PlayerVector<power, MAXPLAYERS> Power;

  // Production statistics per player
  PlayerVector<resource_t, MAXPLAYERS> prod_res;
  PlayerVector<resource_t, MAXPLAYERS> prod_fuel;
  PlayerVector<resource_t, MAXPLAYERS> prod_destruct;
  PlayerVector<resource_t, MAXPLAYERS> prod_crystals;
  PlayerVector<money_t, MAXPLAYERS> prod_money;

  // Average mobility per player
  PlayerVector<unsigned long, MAXPLAYERS> avg_mob;

  // Total production statistics (global accumulators)
  unsigned long tot_resdep{};
  unsigned long prod_eff{};
  unsigned long tot_captured{};
  unsigned long prod_mob{};

  // Inhabited sectors bitmap (one per star)
  std::array<std::uint64_t, NUMSTARS> inhabited{};

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
