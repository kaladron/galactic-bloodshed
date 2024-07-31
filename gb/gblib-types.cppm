// SPDX-License-Identifier: Apache-2.0

export module gblib:types;

import std.compat;

export using shipnum_t = std::uint64_t;
export using starnum_t = std::uint32_t;
export using planetnum_t = std::uint32_t;
export using player_t = std::uint32_t;
export using governor_t = std::uint32_t;
export using segments_t = std::uint32_t;

export using ap_t = std::uint32_t;
export using commodnum_t = std::int64_t;
export using resource_t = std::int64_t;
export using money_t = std::int64_t;
export using population_t = std::int64_t;

export using command_t = std::vector<std::string>;

export enum ScopeLevel { LEVEL_UNIV, LEVEL_STAR, LEVEL_PLAN, LEVEL_SHIP };

export enum PlanetType {
  EARTH = 0,
  ASTEROID = 1,
  MARS = 2,
  ICEBALL = 3,
  GASGIANT = 4,
  WATER = 5,
  FOREST = 6,
  DESERT = 7,
};

export enum SectorType {
  SEC_SEA = 0,
  SEC_LAND = 1,
  SEC_MOUNT = 2,
  SEC_GAS = 3,
  SEC_ICE = 4,
  SEC_FOREST = 5,
  SEC_DESERT = 6,
  SEC_PLATED = 7,
  SEC_WASTED = 8,
};

export struct Commod {
  player_t owner;
  governor_t governor;
  uint8_t type;
  uint64_t amount;
  bool deliver; /* whether the lot is ready for shipping or not */
  money_t bid;
  player_t bidder;
  governor_t bidder_gov;
  starnum_t star_from; /* where the stuff originated from */
  planetnum_t planet_from;
  starnum_t star_to; /* where it goes to */
  planetnum_t planet_to;
};
