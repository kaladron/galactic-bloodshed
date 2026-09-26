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
  PlayerVector<governor_t, MAXPLAYERS> governor{
      Race::leader_id}; /* which subordinate maintains the system */
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

  starnum_t star_id{};
  PlayerVector<PlayerVector<std::uint32_t, MAXPLAYERS>, MAXPLAYERS>
      ground_assaults{}; /* per-turn ground assault tallies [attacker][defender]
                          */
};

export class Star {
public:
  Star(const star_struct& in) : data_(in) {
    if (data_.star_id < 1) {
      throw std::invalid_argument(
          std::format("Star ID must be >= 1 (got {})", data_.star_id));
    }
  }
  explicit Star(starnum_t id, std::string_view name = "",
                UniverseCoordinates coords = {}) {
    if (id < 1) {
      throw std::invalid_argument(
          std::format("Star ID must be >= 1 (got {})", id));
    }
    data_.star_id = id;
    data_.name = name;
    data_.coordinates = coords;
  }

  /// Records a ground assault by `attacker` against `defender` in this star
  /// system during the current turn.
  void record_ground_assault(player_t attacker, player_t defender,
                             std::uint32_t count = 1) {
    data_.ground_assaults[attacker][defender] += count;
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
    return data_.ground_assaults[attacker][defender];
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
    data_.ground_assaults[attacker][defender] = 0;
  }

  /// Clears ground assault tallies between `attacker` and `defender` in this
  /// star system.
  void clear_ground_assaults(const Race& attacker, const Race& defender) {
    clear_ground_assaults(attacker.Playernum, defender.Playernum);
  }

  /// Resets all ground assault tallies in this star system to zero.
  void clear_all_ground_assaults() noexcept {
    data_.ground_assaults = {};
  }

  [[nodiscard]] std::string get_name() const {
    return data_.name;
  }
  void set_name(std::string_view name) {
    data_.name = name;
  }

  [[nodiscard]] const std::string& get_planet_name(planetnum_t pnum) const {
    if (pnum.value < 1 || pnum.value > data_.pnames.size()) {
      throw std::runtime_error(
          std::format("Planet number {} out of range for star '{}' (has {} "
                      "planets)",
                      pnum, data_.name, data_.pnames.size()));
    }
    return data_.pnames[pnum.value - 1];
  }
  void set_planet_name(planetnum_t pnum, std::string_view name) {
    if (pnum.value < 1) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (must be >= 1)", pnum,
          data_.name));
    }
    // Resize vector if necessary to accommodate the 1-based planet number
    if (pnum.value > data_.pnames.size()) {
      data_.pnames.resize(pnum.value);
    }
    data_.pnames[pnum.value - 1] = name;
  }
  [[nodiscard]] bool planet_name_isset(planetnum_t pnum) const {
    if (pnum.value < 1 || pnum.value > data_.pnames.size()) {
      throw std::runtime_error(
          std::format("Planet number {} out of range for star '{}' (has {} "
                      "planets)",
                      pnum, data_.name, data_.pnames.size()));
    }
    return !data_.pnames[pnum.value - 1].empty();
  }

  [[nodiscard]] const std::vector<std::string>& planet_names() const noexcept {
    return data_.pnames;
  }

  PlayerBitset<MAXPLAYERS>& explored() noexcept {
    return data_.explored;
  }
  [[nodiscard]] const PlayerBitset<MAXPLAYERS>& explored() const noexcept {
    return data_.explored;
  }

  /// Returns whether this star system has been explored by the given player.
  [[nodiscard]] bool is_explored_by(player_t p) const noexcept;

  /// Marks the star system as explored by the given player.
  void mark_explored_by(player_t p) noexcept;

  /// Returns whether any player has explored this star system.
  [[nodiscard]] bool is_explored() const noexcept;

  PlayerBitset<MAXPLAYERS>& inhabited() noexcept {
    return data_.inhabited;
  }
  [[nodiscard]] const PlayerBitset<MAXPLAYERS>& inhabited() const noexcept {
    return data_.inhabited;
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
    return data_.pnames.size();
  }

  /// \brief Returns a random 1-based planet index (1..numplanets).
  [[nodiscard]] planetnum_t get_random_planet_index() const;

  [[nodiscard]] constexpr UniverseCoordinates coordinates() const noexcept {
    return data_.coordinates;
  }
  constexpr UniverseCoordinates& coordinates() noexcept {
    return data_.coordinates;
  }
  constexpr void set_coordinates(UniverseCoordinates coords) noexcept {
    data_.coordinates = coords;
  }

  // Action points (1-indexed via PlayerVector)
  ap_t& AP(player_t playernum) {
    return data_.AP[playernum];
  }
  [[nodiscard]] ap_t AP(player_t playernum) const {
    return data_.AP[playernum];
  }

  // which subordinate maintains the system (1-indexed via PlayerVector)
  governor_t& governor(player_t playernum) {
    return data_.governor[playernum];
  }
  [[nodiscard]] governor_t governor(player_t playernum) const {
    return data_.governor[playernum];
  }

  // how close to nova it is
  int& stability() {
    return data_.stability;
  }
  [[nodiscard]] int stability() const {
    return data_.stability;
  }

  // stage of nova
  int& nova_stage() {
    return data_.nova_stage;
  }
  [[nodiscard]] int nova_stage() const {
    return data_.nova_stage;
  }

  // factor which expresses how hot the star is
  int& temperature() {
    return data_.temperature;
  }
  [[nodiscard]] int temperature() const {
    return data_.temperature;
  }

  // attraction of star in "Standards".
  double& gravity() {
    return data_.gravity;
  }
  [[nodiscard]] double gravity() const {
    return data_.gravity;
  }

  /// Checks whether a player and governor have administrative control of this
  /// star system.
  [[nodiscard]] bool control(player_t, governor_t) const;

  [[nodiscard]] star_struct get_struct() const {
    return data_;
  }

  [[nodiscard]] starnum_t star_id() const {
    return data_.star_id;
  }

private:
  star_struct data_{};
};
