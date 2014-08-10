// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVE_H
#define MOVE_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void arm(int, int, int, int);
void move_popn(int, int, int);
void walk(int, int, int);
int get_move(char, int, int, int *, int *, planettype *);
void ground_attack(racetype *, racetype *, int *, int, population_t *,
                   population_t *, unsigned int, unsigned int, double, double,
                   double *, double *, int *, int *, int *);

#endif  // MOVE_H
