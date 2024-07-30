// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAX_H
#define MAX_H

#include "gb/ships.h"

std::string prin_ship_orbits(const Ship &);

constexpr auto maxsupport(const Race &r, const Sector &s, const double c,
                          const int toxic) {
  if (r.likes[s.condition] == 0) return 0L;
  double a = ((double)s.eff + 1.0) * (double)s.fert;
  double b = (.01 * c);

  auto val = std::lround(a * b * .01 * (100.0 - (double)toxic));

  return val;
}

#endif  // MAX_H
