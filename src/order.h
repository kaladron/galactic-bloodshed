// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef ORDER_H
#define ORDER_H

#include "ships.h"

void give_orders(int, int, int, shiptype *);
void order(int, int, int);
char *prin_ship_dest(int, int, shiptype *);
void route(int, int, int);

#endif  // ORDER_H
