// SPDX-License-Identifier: Apache-2.0

module;

import std;
#undef stdout

module gblib;

Sector& SectorMap::get_random() {
  return get_random(game_rng());
}
