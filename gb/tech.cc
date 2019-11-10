// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* tech.c -- increase investment in technological development. */

import gblib;

#include "gb/tech.h"

import std;

namespace {
const double TECH_INVEST = 0.01;  // invest factor
}

double tech_prod(int investment, population_t popn) {
  double scale = (double)popn / 10000.;
  return (TECH_INVEST * log10((double)investment * scale + 1.0));
}
