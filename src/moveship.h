// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVESHIP_H
#define MOVESHIP_H

#include "ships.h"
#include "vars.h"

void Moveship(shiptype *, int, int, int);
void msg_OOF(shiptype *);
int followable(shiptype *, shiptype *);
int do_merchant(shiptype *, planettype *);

#endif  // MOVESHIP_H
