/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * perm.c -- randomly permute a sector list
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"

#include "rand.p"

struct map {
  char x, y;
} xymap[(MAX_X + 1) * (MAX_Y + 1)];

void PermuteSects(planettype *);
int Getxysect(planettype *, int *, int *, int);

/* make a random list of sectors. */
void PermuteSects(planettype *planet) {
  register int i, j, x, y, t;
  struct map sw;

  t = planet->Maxy * planet->Maxx;

  for (i = x = y = 0; i < t; i++) {
    xymap[i].x = x;
    xymap[i].y = y;
    if (++x >= planet->Maxx)
      x = 0, y++;
  }
  for (i = 0; i < t; i++) {
    sw = xymap[i];
    xymap[i] = xymap[j = int_rand(0, t - 1)];
    xymap[j] = sw;
  }
}

/* get the next x,y sector in the list.  if r=1, reset the counter.
**  increments the counter & returns whether or not this reset it to 0.
*/

int Getxysect(reg planettype *p, reg int *x, reg int *y, reg int r) {
  static int getxy, max;

  if (r) {
    getxy = 0;
    max = p->Maxx * p->Maxy;
  } else {
    *x = xymap[getxy].x;
    *y = xymap[getxy].y;
    if (++getxy > max)
      getxy = 0;
  }
  return getxy;
}
