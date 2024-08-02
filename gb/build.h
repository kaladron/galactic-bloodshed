// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BUILD_H
#define BUILD_H

#include "gb/ships.h"

void upgrade(const command_t &, GameObj &);
void make_mod(const command_t &, GameObj &);
void build(const command_t &, GameObj &);
int Shipcost(ShipType, const Race &);
void sell(const command_t &, GameObj &);
void bid(const command_t &argv, GameObj &);
std::tuple<money_t, double> shipping_cost(starnum_t to, starnum_t from,
                                          money_t value);

extern const char *commod_name[4];

#endif  // BUILD_H
