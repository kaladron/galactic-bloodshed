// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FIRE_H
#define FIRE_H

#include "gb/ships.h"

void fire(const command_t &, GameObj &);
void bombard(const command_t &, GameObj &);
void defend(const command_t &, GameObj &);
void detonate(const command_t &argv, GameObj &);
int retal_strength(const Ship &);
int adjacent(int, int, int, int, const Planet &);

#endif  // FIRE_H
