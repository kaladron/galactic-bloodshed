// SPDX-License-Identifier: Apache-2.0

/// \file gblib-globals.cppm
/// \brief Module interface partition for transitional global arrays.

module;

import std;

export module gblib:globals;

import :misc;
import :planet;
import :race;
import :star;
import :types;
import :universe;

export struct StarAssaultTallies {
  std::array<std::uint32_t, NUMSTARS> data_{};

  [[nodiscard]] constexpr std::uint32_t& operator[](starnum_t s) {
    if (s.value >= NUMSTARS) {
      throw std::out_of_range(std::format("Star index {} out of range (0..{})",
                                          s.value, NUMSTARS - 1));
    }
    return data_[s.value];
  }
  [[nodiscard]] constexpr const std::uint32_t& operator[](starnum_t s) const {
    if (s.value >= NUMSTARS) {
      throw std::out_of_range(std::format("Star index {} out of range (0..{})",
                                          s.value, NUMSTARS - 1));
    }
    return data_[s.value];
  }
};

export using GroundAssaultMatrix =
    PlayerVector<PlayerVector<StarAssaultTallies, MAXPLAYERS>, MAXPLAYERS>;

// Ground assault tracking - modified by commands, reported during turn
// Cannot move to TurnStats because commands need access
export GroundAssaultMatrix ground_assaults;

// Power blocks - computed during turn processing, read by commands (e.g., block
// command)
export power_blocks Power_blocks;

export bool update_flag = false;
