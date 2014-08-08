// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef RAND_H
#define RAND_H

int success(int x);

double double_rand(void);
int int_rand(int, int);
long long_rand(long, long);
int round_rand(double);
int rposneg(void);

#endif  // RAND_H
