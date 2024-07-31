// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAP_H
#define MAP_H

#include "gb/races.h"

extern const char Psymbol[];
extern const char *Planet_types[];

void map(const command_t &, GameObj &);
char desshow(const player_t, const governor_t, const Race &, const Sector &);

#endif  // MAP_H
