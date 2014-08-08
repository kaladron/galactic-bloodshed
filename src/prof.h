// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef PROF_H
#define PROF_H

#include "races.h"

void whois(int, int, int);
void profile(int, int, int);
char *Estimate_f(double, racetype *, int);
char *Estimate_i(int, racetype *, int);
int round_perc(int, racetype *, int);
void treasury(int, int);

#endif  // PROF_H
