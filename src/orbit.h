// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef ORBIT_H
#define ORBIT_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void orbit(int, int, int);
void DispStar(int, int, int, startype *, int, int, char *);
void DispPlanet(int, int, int, planettype *, char *, int, racetype *, char *);
void DispShip(int, int, placetype *, shiptype *, planettype *, int, char *);

#endif // ORBIT_H
