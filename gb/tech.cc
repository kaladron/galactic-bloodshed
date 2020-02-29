// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* tech.c -- increase investment in technological development. */

import gblib;
import std;

#include "gb/tech.h"

namespace {
const double TECH_INVEST = 0.01;  // invest factor
}

double tech_prod(const money_t investment, const population_t popn) {
  double scale = static_cast<double>(popn) / 10000.;
  return (TECH_INVEST * log10(static_cast<double>(investment) * scale + 1.0));
}
