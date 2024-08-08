// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FIRE_H
#define FIRE_H

int retal_strength(const Ship &);
int adjacent(int, int, int, int, const Planet &);
int check_retal_strength(const Ship &ship);
bool has_planet_defense(const shipnum_t shipno, const player_t Playernum);
void check_overload(Ship *ship, int cew, int *strength);

#endif  // FIRE_H
