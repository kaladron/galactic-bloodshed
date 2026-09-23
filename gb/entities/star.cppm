// SPDX-License-Identifier: Apache-2.0

/// \file star.cppm
/// \brief Module interface partition for Star entity and system models.

export module gb.entities:star;

import :race;
import :types;
import :tweakables;
import std;

export struct star_struct {
  std::string name; /* name of star */
  PlayerVector<governor_t, MAXPLAYERS>
      governor; /* which subordinate maintains the system */
  PlayerVector<ap_t, MAXPLAYERS> AP;  /* action pts alotted */
  PlayerBitset<MAXPLAYERS> explored;  /* who's been here */
  PlayerBitset<MAXPLAYERS> inhabited; /* who lives here now */
  UniverseCoordinates coordinates{};

  std::vector<std::string>
      pnames; /* names of planets (vector size = numplanets) */

  int stability{0};    /* how close to nova it is */
  int nova_stage{0};   /* stage of nova */
  int temperature{0};  /* factor which expresses how hot the star is*/
  double gravity{0.0}; /* attraction of star in "Standards". */

  starnum_t star_id{0};
  PlayerVector<PlayerVector<std::uint32_t, MAXPLAYERS>, MAXPLAYERS>
      ground_assaults{}; /* per-turn ground assault tallies [attacker][defender]
                          */
};

export class Star {
public:
  /// Records a ground assault by `attacker` against `defender` in this star
  /// system during the current turn.
  void record_ground_assault(player_t attacker, player_t defender,
                             std::uint32_t count = 1) {
    star_struct.ground_assaults[attacker][defender] += count;
  }

  /// Records a ground assault by `attacker` against `defender` in this star
  /// system during the current turn.
  void record_ground_assault(const Race& attacker, const Race& defender,
                             std::uint32_t count = 1) {
    record_ground_assault(attacker.Playernum, defender.Playernum, count);
  }

  /// Returns the number of ground assaults by `attacker` against `defender` in
  /// this star system during the current turn.
  [[nodiscard]] std::uint32_t ground_assault_count(player_t attacker,
                                                   player_t defender) const {
    return star_struct.ground_assaults[attacker][defender];
  }

  /// Returns the number of ground assaults by `attacker` against `defender` in
  /// this star system during the current turn.
  [[nodiscard]] std::uint32_t ground_assault_count(const Race& attacker,
                                                   const Race& defender) const {
    return ground_assault_count(attacker.Playernum, defender.Playernum);
  }

  /// Clears ground assault tallies between `attacker` and `defender` in this
  /// star system.
  void clear_ground_assaults(player_t attacker, player_t defender) {
    star_struct.ground_assaults[attacker][defender] = 0;
  }

  /// Clears ground assault tallies between `attacker` and `defender` in this
  /// star system.
  void clear_ground_assaults(const Race& attacker, const Race& defender) {
    clear_ground_assaults(attacker.Playernum, defender.Playernum);
  }

  /// Resets all ground assault tallies in this star system to zero.
  void clear_all_ground_assaults() noexcept {
    star_struct.ground_assaults = {};
  }

  [[nodiscard]] std::string get_name() const {
    return star_struct.name;
  }
  void set_name(std::string_view name) {
    star_struct.name = name;
  }

  [[nodiscard]] const std::string& get_planet_name(planetnum_t pnum) const {
    if (pnum.value < 1 || pnum.value > star_struct.pnames.size()) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (has {} planets)", pnum,
          star_struct.name, star_struct.pnames.size()));
    }
    return star_struct.pnames[pnum.value - 1];
  }
  void set_planet_name(planetnum_t pnum, std::string_view name) {
    if (pnum.value < 1) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (must be >= 1)", pnum,
          star_struct.name));
    }
    // Resize vector if necessary to accommodate the 1-based planet number
    if (pnum.value > star_struct.pnames.size()) {
      star_struct.pnames.resize(pnum.value);
    }
    star_struct.pnames[pnum.value - 1] = name;
  }
  [[nodiscard]] bool planet_name_isset(planetnum_t pnum) const {
    if (pnum.value < 1 || pnum.value > star_struct.pnames.size()) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (has {} planets)", pnum,
          star_struct.name, star_struct.pnames.size()));
    }
    return !star_struct.pnames[pnum.value - 1].empty();
  };

  PlayerBitset<MAXPLAYERS>& explored() noexcept {
    return star_struct.explored;
  }
  [[nodiscard]] const PlayerBitset<MAXPLAYERS>& explored() const noexcept {
    return star_struct.explored;
  }

  /// Returns whether this star system has been explored by the given player.
  [[nodiscard]] bool is_explored_by(player_t p) const noexcept;

  /// Marks the star system as explored by the given player.
  void mark_explored_by(player_t p) noexcept;

  /// Returns whether any player has explored this star system.
  [[nodiscard]] bool is_explored() const noexcept;

  PlayerBitset<MAXPLAYERS>& inhabited() noexcept {
    return star_struct.inhabited;
  }
  [[nodiscard]] const PlayerBitset<MAXPLAYERS>& inhabited() const noexcept {
    return star_struct.inhabited;
  }

  /// Returns whether this star system is inhabited by the given player.
  [[nodiscard]] bool is_inhabited_by(player_t p) const noexcept;

  /// Marks the star system as inhabited by the given player.
  void mark_inhabited_by(player_t p) noexcept;

  /// Clears habitation status for the given player.
  void clear_inhabited_by(player_t p) noexcept;

  /// Returns whether any player currently inhabits this star system.
  [[nodiscard]] bool is_inhabited() const noexcept;

  /// Clears all planetary inhabitants across all players from this star system.
  void clear_all_inhabitants() noexcept;

  [[nodiscard]] int numplanets() const {
    return star_struct.pnames.size();
  }

  /// \brief Returns a random 1-based planet index (1..numplanets).
  [[nodiscard]] planetnum_t get_random_planet_index() const;

  [[nodiscard]] constexpr UniverseCoordinates coordinates() const noexcept {
    return star_struct.coordinates;
  }
  constexpr UniverseCoordinates& coordinates() noexcept {
    return star_struct.coordinates;
  }
  constexpr void set_coordinates(UniverseCoordinates coords) noexcept {
    star_struct.coordinates = coords;
  }

  // Action points (1-indexed via PlayerVector)
  ap_t& AP(player_t playernum) {
    return star_struct.AP[playernum];
  }
  [[nodiscard]] ap_t AP(player_t playernum) const {
    return star_struct.AP[playernum];
  }

  // which subordinate maintains the system (1-indexed via PlayerVector)
  governor_t& governor(player_t playernum) {
    return star_struct.governor[playernum];
  }
  [[nodiscard]] governor_t governor(player_t playernum) const {
    return star_struct.governor[playernum];
  }

  // how close to nova it is
  int& stability() {
    return star_struct.stability;
  }
  [[nodiscard]] int stability() const {
    return star_struct.stability;
  }

  // stage of nova
  int& nova_stage() {
    return star_struct.nova_stage;
  }
  [[nodiscard]] int nova_stage() const {
    return star_struct.nova_stage;
  }

  // factor which expresses how hot the star is
  int& temperature() {
    return star_struct.temperature;
  }
  [[nodiscard]] int temperature() const {
    return star_struct.temperature;
  }

  // attraction of star in "Standards".
  double& gravity() {
    return star_struct.gravity;
  }
  [[nodiscard]] double gravity() const {
    return star_struct.gravity;
  }

  /// Checks whether a player and governor have administrative control of this
  /// star system.
  [[nodiscard]] bool control(player_t, governor_t) const;

  [[nodiscard]] star_struct get_struct() const {
    return star_struct;
  }

  [[nodiscard]] starnum_t star_id() const {
    return star_struct.star_id;
  }

  Star(const star_struct& in) : star_struct(in) {}

private:
  star_struct star_struct{};
};
