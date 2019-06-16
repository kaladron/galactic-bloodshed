// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOSECTOR_H
#define DOSECTOR_H

#include "gb/vars.h"

void produce(startype *, const Planet &, Sector &);
void spread(const Planet &, Sector &, int, int, SectorMap &);
void explore(const Planet &, Sector &, int, int, int);

#endif  // DOSECTOR_H
