// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef TWEAKABLES_H
#define TWEAKABLES_H

/* number of global APs each planet is worth */
#define EARTH_POINTS int_rand(5, 8)
// Moved to module:
// constexpr ap_t ASTEROID_POINTS = 1;
#define MARS_POINTS int_rand(2, 3)
#define ICEBALL_POINTS int_rand(2, 3)
#define GASGIANT_POINTS int_rand(8, 20)
#define WATER_POINTS int_rand(2, 3)
#define FOREST_POINTS int_rand(2, 3)
#define DESERT_POINTS int_rand(2, 3)

#endif  // TWEAKABLES_H
