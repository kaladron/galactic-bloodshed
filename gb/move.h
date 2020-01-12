// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVE_H
#define MOVE_H

#include "gb/races.h"
#include "gb/vars.h"

void arm(const command_t &, GameObj &);
void move_popn(const command_t &, GameObj &);
void walk(const command_t &, GameObj &);
int get_move(char, int, int, int *, int *, const Planet &);
void ground_attack(Race &, Race &, int *, int, population_t *, population_t *,
                   unsigned int, unsigned int, double, double, double *,
                   double *, int *, int *, int *);

#endif  // MOVE_H
