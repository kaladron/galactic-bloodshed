// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAX_H
#define MAX_H

#include "races.h"
#include "ships.h"
#include "vars.h"

int maxsupport(racetype *, sectortype *, double, int);
double compatibility(const planettype *, const racetype *);
double gravity(planettype *);
char *prin_ship_orbits(shiptype *);

#endif // MAX_H
