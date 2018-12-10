// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef NAME_H
#define NAME_H

#include "vars.h"

void personal(const command_t &, GameObj &);
void bless(const command_t &, GameObj &);
void insurgency(const command_t &, GameObj &);
void pay(const command_t &, GameObj &);
void give(const command_t &, GameObj &);
void page(int, int, int);
void send_message(int, int, int, int);
void read_messages(int, int, int);
void motto(const command_t &, GameObj &);
void name(int, int, int);
void announce(const command_t &, GameObj &);

#endif  // NAME_H
