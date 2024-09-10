// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MOVE_H
#define MOVE_H

int get_move(char, int, int, int *, int *, const Planet &);
void ground_attack(Race &, Race &, int *, int, population_t *, population_t *,
                   unsigned int, unsigned int, double, double, double *,
                   double *, int *, int *, int *);
void mech_defend(player_t, governor_t, int *, int, const Planet &, int, int,
                 const Sector &);
void mech_attack_people(Ship *, population_t *, population_t *, Race &, Race &,
                        const Sector &, int, int, int, char *, char *);
void people_attack_mech(Ship *, int, int, Race &, Race &, const Sector &, int,
                        int, char *, char *);

#endif  // MOVE_H
