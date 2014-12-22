// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef EXPLORE_H
#define EXPLORE_H

#include "races.h"

#include "races.h"
#include "ships.h"
#include "vars.h"

void colonies(const command_t &argv, const player_t Playernum,
              const governor_t Governor);
void distance(int, int, int);
void star_locations(int, int, int);
void exploration(int, int, int);
void tech_status(int, int, int);

#endif  // EXPLORE_H
