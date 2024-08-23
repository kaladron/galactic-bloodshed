// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FUEL_H
#define FUEL_H

std::tuple<bool, segments_t> do_trip(const Place &, Ship &, double fuel,
                                     double gravity_factor, double x_1,
                                     double y_1);

void fuel_output(GameObj &g, double dist, double fuel, double grav, double mass,
                 segments_t segs, std::string_view plan_buf);

#endif  // FUEL_H
