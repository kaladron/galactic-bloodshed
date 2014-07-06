// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* for power report */

#ifndef POWER_H
#define POWER_H

#include "vars.h"

struct power {
  unsigned long troops;   /* total troops */
  unsigned long popn;     /* total population */
  unsigned long resource; /* total resource in stock */
  unsigned long fuel;
  unsigned long destruct;     /* total dest in stock */
  unsigned short ships_owned; /* # of ships owned */
  unsigned short planets_owned;
  unsigned long sectors_owned;
  unsigned long money;
  unsigned long sum_mob; /* total mobilization */
  unsigned long sum_eff; /* total efficiency */
};

extern struct power Power[MAXPLAYERS];

#endif // POWER_H
