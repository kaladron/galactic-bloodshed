// SPDX-License-Identifier: Apache-2.0

/// \file turnstats.cppm
/// \brief Module interface partition for TurnStats turn-scoped statistics
/// accumulator.

module;

import std;

export module gb.turn:turnstats;

import gb.entities;

/// \brief Per-star population and ship counts for a single player.
export struct StarPlayerStats {
  population_t popn{0};
  std::uint32_t num_ships{0};

  [[nodiscard]] constexpr bool
  operator==(const StarPlayerStats&) const noexcept = default;
};

// TurnStats: Encapsulates per-turn accumulating statistics.
// Passed through doplanet() and doship() to replace global array usage.
// Created fresh at the start of each turn.
export struct TurnStats {
  /// \brief Checks that a player ID is a valid 1-based ID (>= 1), throwing
  /// std::out_of_range if not.
  static constexpr void check_player_bounds(player_t p) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p));
    }
  }

  /// \brief Checks that a star ID is a valid 1-based ID (>= 1), throwing
  /// std::out_of_range if not.
  static constexpr void check_star_bounds(starnum_t snum) {
    if (snum < 1) {
      throw std::out_of_range(
          std::format("Star index {} out of range (must be >= 1)", snum));
    }
  }

  /// \brief Checks that star and planet numbers are valid 1-based IDs (>= 1),
  /// throwing std::out_of_range if not.
  static constexpr void check_planet_bounds(starnum_t snum, planetnum_t pnum) {
    check_star_bounds(snum);
    if (pnum < 1) {
      throw std::out_of_range(
          std::format("Planet index {} out of range (must be >= 1)", pnum));
    }
  }

  // --- Per-Star Player Census ---

  [[nodiscard]] const StarPlayerStats& star_player_stats(starnum_t snum,
                                                         player_t p) const {
    check_star_bounds(snum);
    check_player_bounds(p);
    static constexpr StarPlayerStats default_stats{};
    if (const auto it = star_player_stats_.find({snum, p});
        it != star_player_stats_.end()) {
      return it->second;
    }
    return default_stats;
  }

  /// \brief Accumulates population for a player in the given star system.
  void add_star_popn(starnum_t snum, player_t p, population_t popn) {
    check_star_bounds(snum);
    check_player_bounds(p);
    star_player_stats_[{snum, p}].popn += popn;
  }

  /// \brief Accumulates ship presence for a player in the given star system.
  void add_star_ships(starnum_t snum, player_t p, std::uint32_t ships = 1) {
    check_star_bounds(snum);
    check_player_bounds(p);
    star_player_stats_[{snum, p}].num_ships += ships;
  }

  // --- Planetary Simulation Tracking ---

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

  // --- Per-Player Power & Per-Planet Production / Mobilization / Compatibility
  // ---

  [[nodiscard]] const power& power_stats(player_t p) const {
    check_player_bounds(p);
    static constexpr power default_power{};
    if (const auto it = power_.find(p); it != power_.end()) {
      return it->second;
    }
    return default_power;
  }

  /// \brief Returns a mutable reference to a player's accumulated power entry,
  /// inserting a default entry if none exists.
  [[nodiscard]] power& mutable_power_stats(player_t p) {
    check_player_bounds(p);
    return power_[p];
  }

  [[nodiscard]] const Stockpile& production(player_t p) const {
    check_player_bounds(p);
    static constexpr Stockpile default_prod{};
    if (const auto it = prod_.find(p); it != prod_.end()) {
      return it->second;
    }
    return default_prod;
  }

  /// \brief Accumulates a produced stockpile (resources, destruct, fuel,
  /// crystals) into the owning player's per-planet production totals.
  void record_production(player_t owner, const Stockpile& produced) {
    check_player_bounds(owner);
    prod_[owner] += produced;
  }

  /// \brief Deducts resources from the owning player's per-planet produced
  /// stockpile (e.g. for sector mobilization).
  void spend_produced_resources(player_t owner, resource_t amount) {
    check_player_bounds(owner);
    prod_[owner].resources -= amount;
  }

  /// \brief Extracts and zeroes out the enslaved player's produced resources,
  /// fuel, and destruct for slave tribute diversion (preserving crystals).
  [[nodiscard]] Stockpile extract_slave_tribute(player_t slave) {
    check_player_bounds(slave);
    Stockpile tribute{};
    if (auto it = prod_.find(slave); it != prod_.end()) {
      tribute.resources = std::exchange(it->second.resources, 0);
      tribute.fuel = std::exchange(it->second.fuel, 0);
      tribute.destruct = std::exchange(it->second.destruct, 0);
    }
    return tribute;
  }

  [[nodiscard]] std::uint32_t mob_points(player_t p) const {
    check_player_bounds(p);
    if (const auto it = total_mob_points_.find(p);
        it != total_mob_points_.end()) {
      return it->second;
    }
    return 0;
  }

  /// \brief Accumulates sector mobilization points for a player on the current
  /// planet.
  void add_mob_points(player_t p, std::uint32_t points) {
    check_player_bounds(p);
    total_mob_points_[p] += points;
  }

  [[nodiscard]] double compat(player_t p) const {
    check_player_bounds(p);
    if (const auto it = compat_.find(p); it != compat_.end()) {
      return it->second;
    }
    return 0.0;
  }

  /// \brief Sets a player's planetary compatibility for the current planet.
  void set_compat(player_t p, double val) {
    check_player_bounds(p);
    compat_[p] = val;
  }

  /// \brief Resets per-planet accumulators (`Claims`, `tot_captured`,
  /// `production`, `mob_points`, and `compat`) at the start of a planet pass.
  void reset_planet_accumulators() noexcept {
    Claims = false;
    tot_captured = 0;
    prod_.clear();
    total_mob_points_.clear();
    compat_.clear();
  }

  // Per-planet count of newly captured/spread sectors
  sector_count_t tot_captured{};

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

  std::map<std::pair<starnum_t, player_t>, StarPlayerStats>
      star_player_stats_{};
  std::map<std::pair<starnum_t, planetnum_t>, PlanetTurnInfo>
      planet_turn_info_{};
  std::flat_map<player_t, power> power_{};
  std::flat_map<player_t, Stockpile> prod_{};
  std::flat_map<player_t, std::uint32_t> total_mob_points_{};
  std::flat_map<player_t, double> compat_{};
};
