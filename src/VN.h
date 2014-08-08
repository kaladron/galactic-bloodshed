// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef VN_H
#define VN_H

#include "ships.h"
#include "vars.h"

void do_VN(shiptype *);
void planet_doVN(shiptype *, planettype *);
void order_berserker(shiptype *);
void order_VN(shiptype *);

#endif  // VN_H
