// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOPLANET_H
#define DOPLANET_H

#include "ships.h"
#include "vars.h"

int doplanet(int, planettype *, int);
int moveship_onplanet(shiptype *, planettype *);
void terraform(shiptype *, planettype *);
void plow(shiptype *, planettype *);
void do_dome(shiptype *, planettype *);
void do_quarry(shiptype *, planettype *);
void do_berserker(shiptype *, planettype *);
void do_recover(planettype *, int, int);
double est_production(sectortype *);

#endif // DOPLANET_H
