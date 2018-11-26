// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FIRE_H
#define FIRE_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void fire(int, int, int, int);
void bombard(int, int, int);
void defend(int, int, int);
void detonate(const command_t &argv, const player_t Playernum,
              const governor_t Governor);
int retal_strength(shiptype *);
int adjacent(int, int, int, int, const Planet &);
int landed(shiptype *);
int laser_on(shiptype *);

#endif  // FIRE_H
