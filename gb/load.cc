// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  load.c -- load/unload stuff */

module;

import std.compat;

#include "gb/tweakables.h"

module gblib;

void use_fuel(Ship &s, const double amt) {
  s.fuel -= amt;
  s.mass -= amt * MASS_FUEL;
}

void use_destruct(Ship &s, const int amt) {
  s.destruct -= amt;
  s.mass -= (double)amt * MASS_DESTRUCT;
}

void use_resource(Ship &s, const int amt) {
  s.resource -= amt;
  s.mass -= (double)amt * MASS_RESOURCE;
}

void rcv_fuel(Ship &s, const double amt) {
  s.fuel += amt;
  s.mass += amt * MASS_FUEL;
}

void rcv_resource(Ship &s, const int amt) {
  s.resource += amt;
  s.mass += (double)amt * MASS_RESOURCE;
}

void rcv_destruct(Ship &s, const int amt) {
  s.destruct += amt;
  s.mass += (double)amt * MASS_DESTRUCT;
}

void rcv_popn(Ship &s, const int amt, const double mass) {
  s.popn += amt;
  s.mass += (double)amt * mass;
}

void rcv_troops(Ship &s, const int amt, const double mass) {
  s.troops += amt;
  s.mass += (double)amt * mass;
}
