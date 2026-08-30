// SPDX-License-Identifier: Apache-2.0

/// \file gblib-star.cppm
/// \brief Module interface partition for Star entity and system models.

export module gblib:star;

import :types;
import :tweakables;
import std;

export struct star_struct {
  shipnum_t ships{0}; /* 1st ship in orbit */
  std::string name;   /* name of star */
  PlayerVector<governor_t, MAXPLAYERS>
      governor; /* which subordinate maintains the system */
  PlayerVector<ap_t, MAXPLAYERS> AP; /* action pts alotted */
  std::uint64_t explored{0};         /* who's been here 64 bits*/
  std::uint64_t inhabited{0};        /* who lives here now 64 bits*/
  double xpos{0.0}, ypos{0.0};

  std::vector<std::string>
      pnames; /* names of planets (vector size = numplanets) */

  unsigned char stability{0};   /* how close to nova it is */
  unsigned char nova_stage{0};  /* stage of nova */
  unsigned char temperature{0}; /* factor which expresses how hot the star is*/
  double gravity{0.0};          /* attraction of star in "Standards". */

  starnum_t star_id{0};
};

export class Star {
public:
  [[nodiscard]] std::string get_name() const {
    return star_struct.name;
  }
  void set_name(std::string_view name) {
    star_struct.name = name;
  }

  [[nodiscard]] const std::string& get_planet_name(planetnum_t pnum) const {
    if (pnum.value >= star_struct.pnames.size()) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (has {} planets)", pnum,
          star_struct.name, star_struct.pnames.size()));
    }
    return star_struct.pnames[pnum.value];
  }
  void set_planet_name(planetnum_t pnum, std::string_view name) {
    // Resize vector if necessary to accommodate the planet number
    if (pnum.value >= star_struct.pnames.size()) {
      star_struct.pnames.resize(pnum.value + 1);
    }
    star_struct.pnames[pnum.value] = name;
  }
  [[nodiscard]] bool planet_name_isset(planetnum_t pnum) const {
    if (pnum.value >= star_struct.pnames.size()) {
      throw std::runtime_error(std::format(
          "Planet number {} out of range for star '{}' (has {} planets)", pnum,
          star_struct.name, star_struct.pnames.size()));
    }
    return !star_struct.pnames[pnum.value].empty();
  };

  // This is used both as a boolean and a setter.
  std::uint64_t& explored() {
    return star_struct.explored;
  }
  [[nodiscard]] std::uint64_t explored() const {
    return star_struct.explored;
  }

  /// Returns whether this star system has been explored by the given player.
  [[nodiscard]] bool is_explored_by(player_t p) const noexcept;

  /// Marks the star system as explored by the given player.
  void mark_explored_by(player_t p) noexcept;

  /// Returns whether any player has explored this star system.
  [[nodiscard]] bool is_explored() const noexcept;

  std::uint64_t& inhabited() {
    return star_struct.inhabited;
  }
  [[nodiscard]] std::uint64_t inhabited() const {
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

  /// \brief Returns a random planet index (0..numplanets-1), or std::nullopt if
  /// the star has no planets.
  [[nodiscard]] std::optional<planetnum_t> get_random_planet_index() const;

  shipnum_t& ships() {
    return star_struct.ships;
  }
  [[nodiscard]] shipnum_t ships() const {
    return star_struct.ships;
  }

  double& xpos() {
    return star_struct.xpos;
  }
  [[nodiscard]] double xpos() const {
    return star_struct.xpos;
  }

  double& ypos() {
    return star_struct.ypos;
  }
  [[nodiscard]] double ypos() const {
    return star_struct.ypos;
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
  unsigned char& stability() {
    return star_struct.stability;
  }
  [[nodiscard]] unsigned char stability() const {
    return star_struct.stability;
  }

  // stage of nova
  unsigned char& nova_stage() {
    return star_struct.nova_stage;
  }
  [[nodiscard]] unsigned char nova_stage() const {
    return star_struct.nova_stage;
  }

  // factor which expresses how hot the star is
  unsigned char& temperature() {
    return star_struct.temperature;
  }
  [[nodiscard]] unsigned char temperature() const {
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
