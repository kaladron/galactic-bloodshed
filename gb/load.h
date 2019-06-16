// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LOAD_H
#define LOAD_H

#include "gb/ships.h"
#include "gb/vars.h"

void load(const command_t &, GameObj &);
void jettison(const command_t &, GameObj &);
void dump(const command_t &, GameObj &);
void transfer(const command_t &, GameObj &);
void mount(const command_t &, GameObj &);
void use_fuel(Ship *, double);
void use_destruct(Ship *, int);
void use_resource(Ship *, int);
void rcv_fuel(Ship *, double);
void rcv_resource(Ship *, int);
void rcv_destruct(Ship *, int);
void rcv_popn(Ship *, int, double);
void rcv_troops(Ship *, int, double);

#endif  // LOAD_H
