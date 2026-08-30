// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Load and unload fuel/cargo handling.

module;

import std;

module gblib;

void use_fuel(Ship& s, const fuel_t amt) {
  s.fuel() -= amt;
  s.mass() -= amt * MASS_FUEL;
}

void use_destruct(Ship& s, const resource_t amt) {
  s.destruct() -= static_cast<unsigned short>(amt);
  s.mass() -= static_cast<double>(amt) * MASS_DESTRUCT;
}

void use_resource(Ship& s, const resource_t amt) {
  s.resource() -= amt;
  s.mass() -= static_cast<double>(amt) * MASS_RESOURCE;
}

void rcv_fuel(Ship& s, const fuel_t amt) {
  s.fuel() += amt;
  s.mass() += amt * MASS_FUEL;
}

void rcv_resource(Ship& s, const resource_t amt) {
  s.resource() += amt;
  s.mass() += static_cast<double>(amt) * MASS_RESOURCE;
}

void rcv_destruct(Ship& s, const resource_t amt) {
  s.destruct() += static_cast<unsigned short>(amt);
  s.mass() += static_cast<double>(amt) * MASS_DESTRUCT;
}

void rcv_popn(Ship& s, const population_t amt, const double mass) {
  s.popn() += amt;
  s.mass() += static_cast<double>(amt) * mass;
}

void rcv_troops(Ship& s, const population_t amt, const double mass) {
  s.troops() += amt;
  s.mass() += static_cast<double>(amt) * mass;
}
