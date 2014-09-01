// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOSECTOR_H
#define DOSECTOR_H

#include "vars.h"

void produce(startype *, planettype *, sectortype *);
void spread(planettype *, sector &, int, int, sector_map &);
void explore(planettype *, sectortype *, int, int, int);
void plate(sectortype *);

#endif  // DOSECTOR_H
