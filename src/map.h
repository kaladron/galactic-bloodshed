// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAP_H
#define MAP_H

#include "races.h"
#include "vars.h"

extern const char Psymbol[];
extern const char *Planet_types[];

void map(const command_t &argv, const player_t Playernum,
         const governor_t Governor);
char desshow(const player_t, const governor_t, const planettype *, const int,
             const int, const racetype *);

#endif  // MAP_H
