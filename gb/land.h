// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef LAND_H
#define LAND_H

int docked(Ship *);
int overloaded(Ship *);
std::tuple<bool, int> crash(const Ship &s, const double fuel) noexcept;

#endif  // LAND_H
