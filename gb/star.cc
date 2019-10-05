// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file star.cc
/// \brief Definition for Star Class.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

import gblib;
import std;

#include "gb/star.h"

#include "gb/vars.h"

int control(const Star& star, player_t Playernum, governor_t Governor) {
  return (!Governor || star.governor[Playernum - 1] == Governor);
}
