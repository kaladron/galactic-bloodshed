// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOSHIP_H
#define DOSHIP_H

#include "ships.h"
#include "vars.h"

void doship(shiptype *, int);
void domass(shiptype *);
void doown(shiptype *);
void domissile(shiptype *);
void domine(int, int);
void doabm(shiptype *);
int do_weapon_plant(shiptype *);

#endif  // DOSHIP_H
