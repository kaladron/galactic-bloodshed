// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef PROF_H
#define PROF_H

#include "gb/races.h"
#include "gb/vars.h"

void whois(const command_t &, GameObj &);
void profile(const command_t &, GameObj &);
char *Estimate_i(int, const Race &, unsigned int);
void treasury(const command_t &, GameObj &);

#endif  // PROF_H
