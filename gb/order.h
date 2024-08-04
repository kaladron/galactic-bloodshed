// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef ORDER_H
#define ORDER_H

void order(const command_t &, GameObj &);
std::string prin_ship_dest(const Ship &);
void route(const command_t &, GameObj &);

#endif  // ORDER_H
