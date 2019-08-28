// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/place.h"

#include <sstream>

std::string Dispplace(const Place &where) {
  std::ostringstream out;
  switch (where.level) {
    case ScopeLevel::LEVEL_STAR:
      out << "/" << Stars[where.snum]->name;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << Stars[where.snum]->name << "/"
          << Stars[where.snum]->pnames[where.pnum];
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << where.shipno;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      return out.str();
  }
}
