// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef DOTURN_H
#define DOTURN_H

#include "gb/tweakables.h"

struct stinfo {
  short temp_add; /* addition to temperature to each planet */
  unsigned char Thing_add;
  /* new Thing colony on this planet */
  unsigned char inhab;       /* explored by anybody */
  unsigned char intimidated; /* assault platform is here */
};

extern struct stinfo Stinfo[NUMSTARS][MAXPLANETS];

struct vnbrain {
  unsigned short Total_mad; /* total # of VN's destroyed so far */
  unsigned char Most_mad;   /* player most mad at */
};

extern struct vnbrain VN_brain;

struct sectinfo {
  char explored;      /* sector has been explored */
  unsigned char VN;   /* this sector has a VN */
  unsigned char done; /* this sector has been updated */
};

extern struct sectinfo Sectinfo[MAX_X][MAX_Y];

#endif  // DOTURN_H
