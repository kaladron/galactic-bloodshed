// SPDX-License-Identifier: Apache-2.0

/// \file load.cc
/// \brief Load and unload fuel/cargo handling.

module;

import std;

module gblib;

void use_fuel(Ship& s, const fuel_t amt) {
  s.consume_fuel(amt);
}

void use_destruct(Ship& s, const resource_t amt) {
  s.consume_destruct(amt);
}

void use_resource(Ship& s, const resource_t amt) {
  s.consume_resource(amt);
}

void rcv_fuel(Ship& s, const fuel_t amt) {
  s.add_fuel(amt);
}

void rcv_resource(Ship& s, const resource_t amt) {
  s.add_resource(amt);
}

void rcv_destruct(Ship& s, const resource_t amt) {
  s.add_destruct(amt);
}

void rcv_popn(Ship& s, const population_t amt, const double mass) {
  s.add_popn(amt, mass);
}

void rcv_troops(Ship& s, const population_t amt, const double mass) {
  s.add_troops(amt, mass);
}
