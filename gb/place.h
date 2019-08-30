// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef PLACE_H
#define PLACE_H

#include "gb/vars.h"

class Place { /* used in function return for finding place */
 public:
  starnum_t snum;
  planetnum_t pnum;
  shipnum_t shipno;
  ScopeLevel level;      /* .level */
  unsigned char err = 0; /* if error */
  std::string to_string();
};

#endif  // PLACE_H
