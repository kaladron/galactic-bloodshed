// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef PLACE_H
#define PLACE_H

#include "gb/vars.h"

class Place { /* used in function return for finding place */
 public:
  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_,
        shipnum_t shipno_)
      : level(level_), snum(snum_), pnum(pnum_), shipno(shipno_) {}

  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_);

  Place(GameObj&, const std::string&, bool ignore_explore = false);
  ScopeLevel level;
  starnum_t snum;
  planetnum_t pnum;
  shipnum_t shipno;
  unsigned char err = 0; /* if error */
  std::string to_string();
};

#endif  // PLACE_H
