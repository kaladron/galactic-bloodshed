// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* perm.c -- randomly permute a sector list */

#include "gb/perm.h"

#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static struct map { char x, y; } xymap[(MAX_X + 1) * (MAX_Y + 1)];

/* make a random list of sectors. */
void PermuteSects(const Planet &planet) {
  int t = planet.Maxy * planet.Maxx;

  for (int i = 0, x = 0, y = 0; i < t; i++) {
    xymap[i].x = x;
    xymap[i].y = y;
    if (++x >= planet.Maxx) {
      x = 0;
      y++;
    }
  }
  for (int i = 0; i < t; i++) {
    struct map sw = xymap[i];
    int j = int_rand(0, t - 1);
    xymap[i] = xymap[j];
    xymap[j] = sw;
  }
}

/* get the next x,y sector in the list.  if r=1, reset the counter.
 *  increments the counter & returns whether or not this reset it to 0.
 */

int Getxysect(const Planet &p, int *x, int *y, int r) {
  static int getxy;
  static int max;

  if (r) {
    getxy = 0;
    max = p.Maxx * p.Maxy;
  } else {
    *x = xymap[getxy].x;
    *y = xymap[getxy].y;
    if (++getxy > max) getxy = 0;
  }
  return getxy;
}
