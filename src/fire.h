// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FIRE_H
#define FIRE_H

#include "vars.h"
#include "ships.h"

void fire(int, int, int, int);
void bombard(int, int, int);
void defend(int, int, int);
void detonate(int, int, int);
int retal_strength(shiptype *);
int adjacent(int, int, int, int, planettype *);
int landed(shiptype *);
int laser_on(shiptype *);

#endif  // FIRE_H
