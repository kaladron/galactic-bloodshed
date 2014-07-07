// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOTURNCMD_H
#define DOTURNCMD_H

#include "config.h"
#include "races.h"
#include "ships.h"
#include "vars.h"

void do_turn(int);
int APadd(int, int, racetype *);
int governed(racetype *);
void fix_stability(startype *);
void do_reset(int);
void handle_victory(void);
void make_discoveries(racetype *);
#ifdef MARKET
void maintain(racetype *, int, int);
#endif
int attack_planet(shiptype *);
void output_ground_attacks(void);
int planet_points(planettype *);

#endif // DOTURNCMD_H
