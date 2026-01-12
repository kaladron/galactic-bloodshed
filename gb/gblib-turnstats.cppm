// SPDX-License-Identifier: Apache-2.0

// TurnStats: Turn-scoped statistics accumulator
//
// This struct holds arrays that accumulate player statistics during
// a single turn. It is passed through doplanet() and doship() to avoid
// using global arrays for turn processing.
//
// The stats are reset at the start of each turn and used to update
// power rankings, AP calculations, and other per-turn computations.

module;

import std;

export module gblib:turnstats;

import :race;
import :types;

// TurnStats: Encapsulates per-turn accumulating statistics.
// Passed through doplanet() and doship() to replace global array usage.
// Created fresh at the start of each turn; value-initialization zeros all
// arrays.
export struct TurnStats {
  // Per-star population counts for each player
  std::array<std::array<unsigned long, MAXPLAYERS>, NUMSTARS> starpopns{};

  // Per-star ship counts for each player
  std::array<std::array<unsigned short, MAXPLAYERS>, NUMSTARS> starnumships{};

  // Global ship counts per player (for Sdata)
  std::array<unsigned short, MAXPLAYERS> Sdatanumships{};

  // Global population counts per player (for Sdata)
  std::array<unsigned long, MAXPLAYERS> Sdatapopns{};

  // Star info (per star, per planet) - temperature modifications, intimidation
  std::array<std::array<stinfo, MAXPLANETS>, NUMSTARS> Stinfo{};

  // Stars inhabited bitmap (one per star)
  std::array<unsigned long, NUMSTARS> StarsInhab{};

  // Stars explored bitmap (one per star)
  std::array<unsigned long, NUMSTARS> StarsExpl{};

  // Power statistics for each player
  std::array<power, MAXPLAYERS> Power{};

  // Production statistics per player
  std::array<unsigned long, MAXPLAYERS> prod_res{};
  std::array<unsigned long, MAXPLAYERS> prod_fuel{};
  std::array<unsigned long, MAXPLAYERS> prod_destruct{};
  std::array<unsigned long, MAXPLAYERS> prod_crystals{};
  std::array<unsigned long, MAXPLAYERS> prod_money{};

  // Average mobility per player
  std::array<unsigned long, MAXPLAYERS> avg_mob{};

  // Total production statistics (global accumulators)
  unsigned long tot_resdep{};
  unsigned long prod_eff{};
  unsigned long tot_captured{};
  unsigned long prod_mob{};

  // Inhabited sectors bitmap (one per star)
  std::array<uint64_t, NUMSTARS> inhabited{};

  // Compatibility values per player (computed at planet start)
  std::array<double, MAXPLAYERS> Compat{};

  // Claims flag (set if any sector ownership changes)
  bool Claims{};

  // VN brain state (VN AI state per turn)
  vnbrain VN_brain{};

  // Sector processing info (per sector on current planet)
  std::array<std::array<sectinfo, MAX_Y>, MAX_X> Sectinfo{};

  // Non-copyable to prevent accidental copies of large arrays
  TurnStats(const TurnStats&) = delete;
  TurnStats& operator=(const TurnStats&) = delete;

  // Default constructor value-initializes (zeros) all arrays
  TurnStats() = default;

  // Movable for container usage if needed
  TurnStats(TurnStats&&) = default;
  TurnStats& operator=(TurnStats&&) = default;
};
