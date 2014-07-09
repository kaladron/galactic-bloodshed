// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef RST_H
#define RST_H

void rst(int, int, int, int);
void ship_report(int, int, int, unsigned char[]);
void plan_getrships(int, int, int, int);
void star_getrships(int, int, int);
int Getrship(int, int, int);
void Free_rlist(void);
int listed(int, char *);

#endif // RST_H
