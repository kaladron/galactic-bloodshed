// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/tweakables.h"
#include "gb/vars.h"

//* Return gravity for the Planet
double Planet::gravity() const {
  return (double)Maxx * (double)Maxy * GRAV_FACTOR;
}
