// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVE_H
#define MOVE_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void arm(const command_t &argv, const player_t Playernum,
         const governor_t Governor);
void move_popn(const command_t &argv, const player_t Playernum,
               const governor_t Governor);
void walk(const command_t &argv, const player_t Playernum,
          const governor_t Governor);
int get_move(char, int, int, int *, int *, const Planet &);
void ground_attack(racetype *, racetype *, int *, int, population_t *,
                   population_t *, unsigned int, unsigned int, double, double,
                   double *, double *, int *, int *, int *);

#endif  // MOVE_H
