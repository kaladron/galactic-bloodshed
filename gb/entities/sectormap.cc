// SPDX-License-Identifier: Apache-2.0

/// \file sectormap.cc
/// \brief SectorMap container methods and random sector selection.

module;

import std;

module gb.entities;

Sector& SectorMap::get_random() {
  return get_random(game_rng());
}

const Sector& SectorMap::get_random() const {
  return get_random(game_rng());
}

bool SectorMap::process_supernova_devastation(const Star& star) {
  if (!star.nova_stage()) {
    return false;
  }
  bool affected = false;
  for (Sector& p : occupied()) {
    p.apply_supernova(star.nova_stage());
    affected = true;
  }
  return affected;
}
