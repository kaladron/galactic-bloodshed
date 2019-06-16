// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAX_H
#define MAX_H

#include "gb/races.h"
#include "gb/ships.h"
#include "gb/vars.h"

int maxsupport(const Race *r, const Sector &s, const double c, const int toxic);
double compatibility(const Planet &, const Race *);
double gravity(const Planet &);
char *prin_ship_orbits(Ship *);

#endif  // MAX_H
