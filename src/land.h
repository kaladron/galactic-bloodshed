// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LAND_H
#define LAND_H

#include "ships.h"

void land(player_t, governor_t, int);
int crash(shiptype *, double);
int docked(shiptype *);
int overloaded(shiptype *);

#endif  // LAND_H
