// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LOAD_H
#define LOAD_H

#include "ships.h"
#include "vars.h"

void load(const command_t &, GameObj &);
void jettison(const command_t &, GameObj &);
void dump(const command_t &, GameObj &);
void transfer(const command_t &, GameObj &);
void mount(const command_t &, GameObj &);
void use_fuel(shiptype *, double);
void use_destruct(shiptype *, int);
void use_resource(shiptype *, int);
void rcv_fuel(shiptype *, double);
void rcv_resource(shiptype *, int);
void rcv_destruct(shiptype *, int);
void rcv_popn(shiptype *, int, double);
void rcv_troops(shiptype *, int, double);

#endif  // LOAD_H
