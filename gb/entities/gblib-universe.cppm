// SPDX-License-Identifier: Apache-2.0

/// \file gblib-universe.cppm
/// \brief Module interface partition for Universe entity and game-wide
/// statistics.

export module gblib:universe;

import :types;
import :tweakables;
import std;

// Underlying universe-level singleton data structure
// This was previously called "stardata" but that name was confusing
// as it contains universe-wide data, not star-specific data
export struct universe_struct {
  int id{0};  // Universe ID for database persistence (always 1 for singleton)
  std::uint32_t numstars{0};   /* Total # of stars in universe */
  shipnum_t ships{0};          /* Head of universe-wide ship list */
  planetnum_t planet_count{0}; /* Count of non-asteroid planets (for victory) */
  PlayerVector<ap_t, MAXPLAYERS> AP;
  PlayerVector<std::uint32_t, MAXPLAYERS> VN_hitlist;
  /* # of ships destroyed by each player */
  PlayerVector<int, MAXPLAYERS> VN_index1; /* negative value is used */
  PlayerVector<int, MAXPLAYERS> VN_index2; /* VN's record of destroyed ships
                                              systems where they bought it */
};

// Wrapper class for Universe data (like Star wraps star_struct)
// Provides type-safe accessor methods instead of raw array access
export class Universe {
  universe_struct& data;

public:
  explicit Universe(universe_struct& raw_data) : data(raw_data) {}

  // Basic accessors
  [[nodiscard]] std::uint32_t numstars() const {
    return data.numstars;
  }
  void set_numstars(std::uint32_t value) {
    data.numstars = value;
  }

  [[nodiscard]] shipnum_t ships() const {
    return data.ships;
  }
  void set_ships(shipnum_t value) {
    data.ships = value;
  }

  // Action Point (AP) methods
  [[nodiscard]] ap_t get_AP(player_t p) const {
    return data.AP[p];
  }

  void set_AP(player_t p, ap_t value) {
    data.AP[p] = value;
  }

  void deduct_AP(player_t p, ap_t amount) {
    data.AP[p] = (data.AP[p] > amount) ? (data.AP[p] - amount) : 0;
  }

  void add_AP(player_t p, ap_t amount) {
    data.AP[p] += amount;
  }

  // VN (Von Neumann) tracking methods
  [[nodiscard]] std::uint32_t get_VN_hitlist(player_t p) const {
    return data.VN_hitlist[p];
  }

  void set_VN_hitlist(player_t p, std::uint32_t value) {
    data.VN_hitlist[p] = value;
  }

  void increment_VN_hitlist(player_t p) {
    data.VN_hitlist[p]++;
  }

  void decrement_VN_hitlist(player_t p) {
    if (data.VN_hitlist[p] > 0) data.VN_hitlist[p]--;
  }

  [[nodiscard]] int get_VN_index1(player_t p) const {
    return data.VN_index1[p];
  }

  void set_VN_index1(player_t p, int value) {
    data.VN_index1[p] = value;
  }

  [[nodiscard]] int get_VN_index2(player_t p) const {
    return data.VN_index2[p];
  }

  void set_VN_index2(player_t p, int value) {
    data.VN_index2[p] = value;
  }

  // Direct access to underlying struct (for migration compatibility)
  universe_struct* operator->() {
    return &data;
  }
  const universe_struct* operator->() const {
    return &data;
  }
  universe_struct& operator*() {
    return data;
  }
  const universe_struct& operator*() const {
    return data;
  }
};
