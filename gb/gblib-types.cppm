// SPDX-License-Identifier: Apache-2.0

export module gblib:types;

import std.compat;

export using shipnum_t = uint64_t;
export using starnum_t = uint32_t;
export using planetnum_t = uint32_t;
export using player_t = uint32_t;
export using governor_t = uint32_t;
export using segments_t = uint32_t;

export using ap_t = uint32_t;
export using commodnum_t = int64_t;
export using resource_t = int64_t;
export using money_t = int64_t;
export using population_t = int64_t;

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
