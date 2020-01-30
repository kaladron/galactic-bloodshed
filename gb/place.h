// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef PLACE_H
#define PLACE_H

class Place { /* used in function return for finding place */
 public:
  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_,
        shipnum_t shipno_)
      : level(level_), snum(snum_), pnum(pnum_), shipno(shipno_) {}

  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_);

  Place(GameObj&, std::string_view, bool ignore_explore = false);
  ScopeLevel level;
  starnum_t snum;
  planetnum_t pnum;
  shipnum_t shipno;
  bool err = false;
  std::string to_string();

 private:
  void getplace2(GameObj& g, std::string_view string, const bool ignoreexpl);
};

#endif  // PLACE_H
