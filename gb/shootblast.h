// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHOOTBLAST_H
#define SHOOTBLAST_H

#include "config.h"
#include "races.h"
#include "ships.h"
#include "vars.h"

int shoot_ship_to_ship(Ship *, Ship *, int, int, int, char *, char *);
#ifdef DEFENSE
int shoot_planet_to_ship(Race *, Ship *, int, char *, char *);
#endif
int shoot_ship_to_planet(Ship *, Planet *, int, int, int, SectorMap &, int, int,
                         char *, char *);
int hit_odds(double, int *, double, int, int, int, int, int, int, int, int);
double gun_range(Race *, Ship *, int);
double tele_range(int, double);
int current_caliber(Ship *);
void do_collateral(Ship *, int, int *, int *, int *, int *);
int getdefense(Ship *);
int planet_guns(int);

#endif  // SHOOTBLAST_H
