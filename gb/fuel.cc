// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fuel.c -- See estimations in fuel consumption and travel time. */

import gblib;
import std.compat;

#include "gb/fuel.h"

#include "gb/doship.h"
#include "gb/moveship.h"
#include "gb/tweakables.h"

void fuel_output(GameObj &g, double dist, double fuel, double grav, double mass,
                 segments_t segs, std::string_view plan_buf) {
  std::string grav_buf =
      (grav > 0.00)
          ? std::format(" ({:.2f} used to launch from {})\n",
                        grav * mass * LAUNCH_GRAV_MASS_FACTOR, plan_buf)
          : " ";

  g.out << std::format(
      "Total Distance = {:.2f}   Number of Segments = {}\nFuel = {:.2f}{}  ",
      dist, segs, fuel, grav_buf);

  if (nsegments_done > segments) {
    g.out << "Estimated arrival time not available due to segment # "
             "discrepancy.\n";
  } else {
    time_t effective_time =
        next_segment_time + ((segs - 1) * (update_time / segments) * 60);
    if (segments == 1) {
      effective_time = next_update_time +
                       (static_cast<time_t>((segs - 1) * (update_time * 60)));
    }
    g.out << std::format("ESTIMATED Arrival Time: {}\n",
                         std::ctime(&effective_time));
    return;
  }
}

std::tuple<bool, segments_t> do_trip(const Place &tmpdest, Ship &tmpship,
                                     double fuel, double gravity_factor,
                                     double x_1, double y_1) {
  tmpship.fuel = fuel; /* load up the pseudo-ship */
  segments_t effective_segment_number = nsegments_done;

  /*  Set our temporary destination.... */
  tmpship.destshipno = tmpdest.shipno;
  tmpship.whatdest = tmpdest.level;
  tmpship.deststar = tmpdest.snum;
  tmpship.destpnum = tmpdest.pnum;
  if (tmpship.whatdest == ScopeLevel::LEVEL_SHIP || tmpship.ships) {
    /* Bring in the other ships.  moveship() uses ships[]. */
    Num_ships = Numships();
    ships = (Ship **)malloc(sizeof(Ship *) * (Num_ships) + 1);
    for (shipnum_t i = 1; i <= Num_ships; i++) (void)getship(&ships[i], i);
  }

  bool trip_resolved = false;
  segments_t number_segments = 0; /* Reset counter.  */

  /*  Launch the ship if it's on a planet.  */
  double gravity_fuel = gravity_factor * tmpship.mass * LAUNCH_GRAV_MASS_FACTOR;
  tmpship.fuel -= gravity_fuel;
  tmpship.docked = 0;

  while (!trip_resolved) {
    domass(&tmpship);
    double fuel_level1 = tmpship.fuel;
    moveship(&tmpship, (effective_segment_number == segments), 0, 1);
    number_segments++;
    effective_segment_number++;
    if (effective_segment_number == (segments + 1))
      effective_segment_number = 1;
    double x_0 = tmpship.xpos;
    double y_0 = tmpship.ypos;
    double tmpdist = sqrt(Distsq(x_0, y_0, x_1, y_1));
    switch (tmpship.whatdest) {
      case ScopeLevel::LEVEL_STAR:
        if (tmpdist <= (double)SYSTEMSIZE) trip_resolved = true;
        break;
      case ScopeLevel::LEVEL_PLAN:
        if (tmpdist <= (double)PLORBITSIZE) trip_resolved = true;
        break;
      case ScopeLevel::LEVEL_SHIP:
        if (tmpdist <= (double)DIST_TO_LAND) trip_resolved = true;
        break;
      default:
        trip_resolved = true;
    }
    if (((tmpship.fuel == fuel_level1) && (!tmpship.hyper_drive.on)) &&
        (trip_resolved == 0)) {
      if (tmpship.whatdest == ScopeLevel::LEVEL_SHIP) {
        for (shipnum_t i = 1; i <= Num_ships; i++) free(ships[i]);
        free(ships);
      }
      return {false, number_segments};
    }
  }
  if (tmpship.whatdest == ScopeLevel::LEVEL_SHIP || tmpship.ships) {
    for (shipnum_t i = 1; i <= Num_ships; i++) free(ships[i]);
    free(ships);
  }
  return {true, number_segments};
}
