// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LOAD_H
#define LOAD_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void load(int, int, int, int);
void jettison(int, int, int);
void dump(int, int, int);
void transfer(int, int, int);
void mount(int, int, int, int);
void use_fuel(shiptype *, double);
void use_destruct(shiptype *, int);
void use_resource(shiptype *, int);
void use_popn(shiptype *, int, double);
void rcv_fuel(shiptype *, double);
void rcv_resource(shiptype *, int);
void rcv_destruct(shiptype *, int);
void rcv_popn(shiptype *, int, double);
void rcv_troops(shiptype *, int, double);
void do_transporter(racetype *, int, shiptype *);
int landed_on(shiptype *, int);
void unload_onto_alien_sector(int, int, planettype *, shiptype *, sectortype *,
                              int, int);

#endif // LOAD_H
