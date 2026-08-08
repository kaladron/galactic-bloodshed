// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Load and unload fuel/cargo handling.

module;

import std;

module gblib;

void use_fuel(Ship& s, const double amt) {
  s.fuel() -= amt;
  s.mass() -= amt * MASS_FUEL;
}

void use_destruct(Ship& s, const int amt) {
  s.destruct() -= amt;
  s.mass() -= (double)amt * MASS_DESTRUCT;
}

void use_resource(Ship& s, const int amt) {
  s.resource() -= amt;
  s.mass() -= (double)amt * MASS_RESOURCE;
}

void rcv_fuel(Ship& s, const double amt) {
  s.fuel() += amt;
  s.mass() += amt * MASS_FUEL;
}

void rcv_resource(Ship& s, const int amt) {
  s.resource() += amt;
  s.mass() += (double)amt * MASS_RESOURCE;
}

void rcv_destruct(Ship& s, const int amt) {
  s.destruct() += amt;
  s.mass() += (double)amt * MASS_DESTRUCT;
}

void rcv_popn(Ship& s, const int amt, const double mass) {
  s.popn() += amt;
  s.mass() += (double)amt * mass;
}

void rcv_troops(Ship& s, const int amt, const double mass) {
  s.troops() += amt;
  s.mass() += (double)amt * mass;
}
