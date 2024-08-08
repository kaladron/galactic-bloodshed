// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAP_H
#define MAP_H

extern const char Psymbol[];
extern const char *Planet_types[];

char desshow(const player_t, const governor_t, const Race &, const Sector &);
void show_map(const player_t, const governor_t, const starnum_t,
              const planetnum_t, const Planet &);

#endif  // MAP_H
