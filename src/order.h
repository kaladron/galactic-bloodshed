// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef ORDER_H
#define ORDER_H

#include <string>

#include "ships.h"

void give_orders(int, int, int, shiptype *);
void order(int, int, int);
std::string prin_ship_dest(const ship &);
void route(int, int, int);

#endif  // ORDER_H
