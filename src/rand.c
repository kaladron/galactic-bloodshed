/*
** Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
** smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
** Restrictions in GB_copyright.h.
**
**   Random number generator
**
**	double double_rand() this returns a random number between 0 and 1
**
**	int int_rand(low,hi) -	this returns an integer random number
**				between hi and low, inclusive.
**
**	int round_rand(double) - returns double rounded to integer, with
**				proportional chance of rounding up or
**				down.
**
**	int rposneg() - either -1 or 1
*/

#include "GB_copyright.h"

double double_rand(void);
int int_rand(int, int);
int round_rand(double);
int rposneg(void);

long random(void);

double double_rand(void) { return (double)random() / 2147483648.0; }

int int_rand(int low, int hi) {
  return ((hi <= low) ? low : (random() % (hi - low + 1)) + low);
}

int round_rand(double x) {
  return ((double_rand() > (x - (double)((int)x))) ? (int)x : (int)(x + 1));
}

int rposneg(void) { return ((random() & 1) ? -1 : 1); }
