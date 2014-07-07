// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MAP_H
#define MAP_H

extern const char Psymbol[];
extern const char *Planet_types[];

void map(int, int, int);
void show_map(int, int, int, int, planettype *, int, int);
char desshow(int, int, planettype *, int, int, racetype *);

#endif // MAP_H
