// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module gblib;

import std;

std::ostream &operator<<(std::ostream &os, const Sector &s) {
  os << "Efficiency: " << s.eff << std::endl;
  os << "Fertility: " << s.fert << std::endl;
  os << "Mobilization: " << s.mobilization << std::endl;
  os << "Crystals: " << s.crystals << std::endl;
  os << "Resource: " << s.resource << std::endl;
  os << "Population: " << s.popn << std::endl;
  os << "Troops: " << s.troops << std::endl;
  os << "Owner: " << s.owner << std::endl;
  os << "Race: " << s.race << std::endl;
  os << "Type: " << s.type << std::endl;
  os << "Condition: " << s.condition << std::endl;
  return os;
}
