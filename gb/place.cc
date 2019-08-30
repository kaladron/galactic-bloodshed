// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/place.h"

#include <sstream>

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
