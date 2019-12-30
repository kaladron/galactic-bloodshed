// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/races.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

//* Return gravity for the Planet
double Planet::gravity() const {
  return (double)Maxx * (double)Maxy * GRAV_FACTOR;
}

double Planet::compatibility(const Race &race) const {
  double atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  int add = 0.1 * ((double)conditions[TEMP] - race.conditions[TEMP]);
  double sum = 1.0 - (double)abs(add) / 100.0;

  /* step through and report compatibility of each planetary gas */
  for (int i = TEMP + 1; i <= OTHER; i++) {
    add = (double)conditions[i] - race.conditions[i];
    atmosphere *= 1.0 - (double)abs(add) / 100.0;
  }
  sum *= atmosphere;
  sum *= 100.0 - conditions[TOXIC];

  if (sum < 0.0) return 0.0;
  return sum;
}

int Planet::get_points() const {
  switch (type) {
    case PlanetType::ASTEROID:
      return ASTEROID_POINTS;
    case PlanetType::EARTH:
      return EARTH_POINTS;
    case PlanetType::MARS:
      return MARS_POINTS;
    case PlanetType::ICEBALL:
      return ICEBALL_POINTS;
    case PlanetType::GASGIANT:
      return GASGIANT_POINTS;
    case PlanetType::WATER:
      return WATER_POINTS;
    case PlanetType::FOREST:
      return FOREST_POINTS;
    case PlanetType::DESERT:
      return DESERT_POINTS;
  }
}
