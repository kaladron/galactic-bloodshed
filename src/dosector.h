// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOSECTOR_H
#define DOSECTOR_H

#include "vars.h"

void produce(startype *, const planet &, sector &);
void spread(const planet &, sector &, int, int, sector_map &);
void explore(const planet &, sector &, int, int, int);

#endif  // DOSECTOR_H
