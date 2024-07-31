// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  main bunch of variables */

#ifndef VARS_H
#define VARS_H

#include "gb/tweakables.h"

#define M_FUEL 0x1
#define M_DESTRUCT 0x2
#define M_RESOURCES 0x4
#define M_CRYSTALS 0x8
#define Fuel(x) ((x) & M_FUEL)
#define Destruct(x) ((x) & M_DESTRUCT)
#define Resources(x) ((x) & M_RESOURCES)
#define Crystals(x) ((x) & M_CRYSTALS)

#endif  // VARS_H
