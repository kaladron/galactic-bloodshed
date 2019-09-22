// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/place.h"

#include <sstream>
#include "gb/getplace.h"

Place::Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_)
    : level(level_), snum(snum_), pnum(pnum_), shipno(0) {
  if (level_ == ScopeLevel::LEVEL_SHIP)
    throw std::runtime_error("Must give ship number when level is a ship");
}

std::string Place::to_string() {
  std::ostringstream out;
  switch (level) {
    case ScopeLevel::LEVEL_STAR:
      out << "/" << Stars[snum]->name;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << Stars[snum]->name << "/" << Stars[snum]->pnames[pnum];
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << shipno;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      return out.str();
  }
}

Place::Place(GameObj& g, const std::string& in, bool ignore_explore) {
  auto tmp = getplace(g, in, ignore_explore);
  level = tmp.level;
  snum = tmp.snum;
  pnum = tmp.pnum;
  shipno = tmp.shipno;
  err = tmp.err;
}
