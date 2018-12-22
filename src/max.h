// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAX_H
#define MAX_H

#include "races.h"
#include "ships.h"
#include "vars.h"

int maxsupport(const racetype *r, const sector &s, const double c,
               const int toxic);
double compatibility(const Planet &, const racetype *);
double gravity(const Planet &);
char *prin_ship_orbits(Ship *);

#endif  // MAX_H
