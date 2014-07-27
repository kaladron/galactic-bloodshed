// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BUILD_H
#define BUILD_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void upgrade(const command_t &argv, const player_t Playernum,
             const governor_t Governor);
void make_mod(const command_t &argv, const player_t Playernum,
              const governor_t Governor);
void build(const command_t &argv, const player_t Playernum,
           const governor_t Governor);
double cost(shiptype *);
double getmass(shiptype *);
int ship_size(shiptype *);
double complexity(shiptype *);
int Shipcost(int, racetype *);
void sell(const command_t &argv, const player_t Playernum,
          const governor_t Governor);
void bid(const command_t &argv, const player_t Playernum,
         const governor_t Governor);
int shipping_cost(int, int, double *, int);

extern const char *Commod[4];

#endif // BUILD_H
