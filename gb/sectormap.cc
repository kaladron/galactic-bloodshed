// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import std;
import gblib;

#include "gb/utils/rand.h"
#include "gb/vars.h"

Sector &SectorMap::get_random() {
  return get(int_rand(0, maxx_ - 1), int_rand(0, maxy_ - 1));
}

std::vector<std::reference_wrapper<Sector>> SectorMap::shuffle() {
  std::random_device rd;
  std::default_random_engine g(rd());

  std::vector<std::reference_wrapper<Sector>> shuffled(vec_.begin(),
                                                       vec_.end());
  std::shuffle(shuffled.begin(), shuffled.end(), g);

  return shuffled;
}
