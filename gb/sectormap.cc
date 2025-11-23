// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

Sector& SectorMap::get_random() {
  return get(int_rand(0, maxx_ - 1), int_rand(0, maxy_ - 1));
}

std::vector<std::reference_wrapper<Sector>> SectorMap::shuffle() {
  std::random_device rd;
  std::default_random_engine g(rd());

  std::vector<std::reference_wrapper<Sector>> shuffled(vec_.begin(),
                                                       vec_.end());
  std::ranges::shuffle(shuffled, g);

  return shuffled;
}
