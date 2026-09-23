// SPDX-License-Identifier: Apache-2.0

/// \file universe.cppm
/// \brief Module interface partition for Universe entity and game-wide
/// statistics.

export module gb.entities:universe;

import :types;
import :tweakables;
import std;

// Underlying universe-level singleton data structure
// This was previously called "stardata" but that name was confusing
// as it contains universe-wide data, not star-specific data
export struct universe_struct {
  PlayerVector<ap_t, MAXPLAYERS> AP;
  PlayerVector<std::uint32_t, MAXPLAYERS> VN_hitlist;
  /* # of ships destroyed by each player */
  PlayerVector<std::optional<starnum_t>, MAXPLAYERS> VN_index1;
  PlayerVector<std::optional<starnum_t>, MAXPLAYERS> VN_index2;
  /* VN's record of destroyed ships systems where they bought it */
};

// Wrapper class for Universe data (like Star wraps star_struct)
// Provides type-safe accessor methods instead of raw array access
export class Universe {
  universe_struct& data;

public:
  explicit Universe(universe_struct& raw_data) : data(raw_data) {}

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

  [[nodiscard]] std::optional<starnum_t> get_VN_index1(player_t p) const {
    return data.VN_index1[p];
  }

  void set_VN_index1(player_t p, std::optional<starnum_t> value) {
    data.VN_index1[p] = value;
  }

  [[nodiscard]] std::optional<starnum_t> get_VN_index2(player_t p) const {
    return data.VN_index2[p];
  }

  void set_VN_index2(player_t p, std::optional<starnum_t> value) {
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
