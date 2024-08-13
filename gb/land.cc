// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  land.c -- land a ship
 *  also.... dock -- dock a ship w/ another ship
 *  and..... assault -- a very un-PC version of land/dock
 */

import gblib;
import std.compat;

#include "gb/land.h"

/// Determine whether the ship crashed or not.
std::tuple<bool, int> crash(const Ship &s, const double fuel) noexcept {
  // Crash from insufficient fuel.
  if (s.fuel < fuel) return {true, 0};

  // Damaged ships stand of chance of crash landing.
  if (auto roll = int_rand(1, 100); roll <= s.damage) return {true, roll};

  // No crash.
  return {false, 0};
}

int docked(const Ship &s) {
  return s.docked && s.whatdest == ScopeLevel::LEVEL_SHIP;
}

int overloaded(const Ship &s) {
  return (s.resource > max_resource(s)) || (s.fuel > max_fuel(s)) ||
         (s.popn + s.troops > s.max_crew) || (s.destruct > max_destruct(s));
}
