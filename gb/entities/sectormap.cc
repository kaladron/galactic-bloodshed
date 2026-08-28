// SPDX-License-Identifier: Apache-2.0

/// \file sectormap.cc
/// \brief SectorMap container methods and random sector selection.

module;

import std;
#undef stdout

module gblib;

Sector& SectorMap::get_random() {
  return get_random(game_rng());
}

const Sector& SectorMap::get_random() const {
  return get_random(game_rng());
}
