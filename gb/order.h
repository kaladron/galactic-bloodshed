// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef ORDER_H
#define ORDER_H

void DispOrders(int Playernum, int Governor, Ship &ship);
void DispOrdersHeader(int Playernum, int Governor);
void give_orders(GameObj &, const command_t &, int, Ship *);

#endif  // ORDER_H
