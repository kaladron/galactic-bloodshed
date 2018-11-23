// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef CAPTURE_H
#define CAPTURE_H

#include "ships.h"

void capture(const command_t &, const player_t, const governor_t);
void capture_stuff(shiptype *);

#endif  // CAPTURE_H
