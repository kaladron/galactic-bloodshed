// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BUILD_H
#define BUILD_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void upgrade(const command_t &, GameObj &);
void make_mod(const command_t &, GameObj &);
void build(const command_t &, GameObj &);
double cost(Ship *);
double getmass(Ship *);
unsigned int ship_size(Ship *);
double complexity(Ship *);
int Shipcost(ShipType, Race *);
void sell(const command_t &, GameObj &);
void bid(const command_t &argv, GameObj &);
int shipping_cost(int, int, double *, int);

extern const char *Commod[4];

#endif  // BUILD_H
