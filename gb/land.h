// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LAND_H
#define LAND_H

#include "gb/ships.h"
#include "gb/vars.h"

void land(const command_t &, GameObj &);
int docked(Ship *);
int overloaded(Ship *);

#endif  // LAND_H
