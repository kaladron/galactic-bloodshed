// SPDX-License-Identifier: Apache-2.0

/// \file gblib-turnstats.cppm
/// \brief Module interface partition for TurnStats turn-scoped statistics
/// accumulator.

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
  std::array<PlayerVector<unsigned long, MAXPLAYERS>, NUMSTARS> starpopns{};

  // Per-star ship counts for each player
  std::array<PlayerVector<unsigned short, MAXPLAYERS>, NUMSTARS> starnumships{};

  // Global ship counts per player (for Sdata)
  PlayerVector<unsigned short, MAXPLAYERS> Sdatanumships;

  // Global population counts per player (for Sdata)
  PlayerVector<unsigned long, MAXPLAYERS> Sdatapopns;

  // Star info (per star, per planet) - temperature modifications, intimidation
  std::array<std::array<Stinfo, MAXPLANETS>, NUMSTARS> Stinfo{};

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
};
