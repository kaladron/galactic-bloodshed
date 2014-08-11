// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* Random number generator */

#include "rand.h"

#include <stdlib.h>

/* double double_rand() this returns a random number between 0 and 1 */
double double_rand(void) { return (double)random() / 2147483648.0; }

/*	int int_rand(low,hi) -	this returns an integer random number
 *				between hi and low, inclusive. */
long long_rand(long low, long hi) {
  return ((hi <= low) ? low : (random() % (hi - low + 1)) + low);
}

/*	int int_rand(low,hi) -	this returns an integer random number
 *				between hi and low, inclusive. */
int int_rand(int low, int hi) {
  return ((hi <= low) ? low : (random() % (hi - low + 1)) + low);
}

/* int round_rand(double) - returns double rounded to integer, with
 *			proportional chance of rounding up or
 *			down. */
int round_rand(double x) {
  return ((double_rand() > (x - (double)((int)x))) ? (int)x : (int)(x + 1));
}

int success(int x) { return int_rand(1, 100) <= (x); }
