// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVE_H
#define MOVE_H

#include "races.h"
#include "ships.h"
#include "vars.h"

void arm(int, int, int, int);
void move_popn(int, int, int);
void walk(int, int, int);
int get_move(char, int, int, int *, int *, planettype *);
void mech_defend(int, int, int *, int, planettype *, int, int,
                 sectortype *, int, int, sectortype *);
void mech_attack_people(shiptype *, int *, int *, racetype *, racetype *,
                        sectortype *, int, int, int, char *, char *);
void people_attack_mech(shiptype *, int, int, racetype *, racetype *,
                        sectortype *, int, int, char *, char *);
void ground_attack(racetype *, racetype *, int *, int, unsigned short *,
                   unsigned short *, unsigned int, unsigned int, double,
                   double, double *, double *, int *, int *, int *);

#endif // MOVE_H
