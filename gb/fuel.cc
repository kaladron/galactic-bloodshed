// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fuel.c -- See estimations in fuel consumption and travel time. */

import gblib;
import std.compat;

#include "gb/fuel.h"

#include "gb/GB_server.h"
#include "gb/doship.h"
#include "gb/fire.h"
#include "gb/max.h"
#include "gb/moveship.h"
#include "gb/order.h"
#include "gb/place.h"
#include "gb/tweakables.h"

void fuel_output(int Playernum, int Governor, double dist, double fuel,
                 double grav, double mass, segments_t segs) {
  char buf[1024];
  char grav_buf[1024];

  if (grav > 0.00)
    sprintf(grav_buf, " (%.2f used to launch from %s)\n",
            (double)(grav * mass * (double)LAUNCH_GRAV_MASS_FACTOR), plan_buf);
  else
    sprintf(grav_buf, " ");
  sprintf(buf,
          "Total Distance = %.2f   Number of Segments = %d\nFuel = %.2f%s  ",
          dist, segs, fuel, grav_buf);
  notify(Playernum, Governor, buf);
  if (nsegments_done > segments)
    notify(
        Playernum, Governor,
        "Estimated arrival time not available due to segment # discrepancy.\n");
  else {
    time_t effective_time =
        next_segment_time + ((segs - 1) * (update_time / segments) * 60);
    if (segments == 1)
      effective_time =
          next_update_time + (long)((segs - 1) * (update_time * 60));
    sprintf(buf, "ESTIMATED Arrival Time: %s\n", ctime(&effective_time));
    notify(Playernum, Governor, buf);
    return;
  }
}

std::tuple<bool, segments_t> do_trip(const Place &tmpdest, Ship &tmpship,
                                     double fuel, double gravity_factor,
                                     double x_1, double y_1) {
  segments_t effective_segment_number;
  int trip_resolved;
  double gravity_fuel;
  double tmpdist;
  double fuel_level1;

  tmpship.fuel = fuel; /* load up the pseudo-ship */
  effective_segment_number = nsegments_done;

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

  trip_resolved = 0;
  segments_t number_segments = 0; /* Reset counter.  */

  /*  Launch the ship if it's on a planet.  */
  gravity_fuel = gravity_factor * tmpship.mass * LAUNCH_GRAV_MASS_FACTOR;
  tmpship.fuel -= gravity_fuel;
  tmpship.docked = 0;

  while (trip_resolved == 0) {
    domass(&tmpship);
    fuel_level1 = tmpship.fuel;
    moveship(&tmpship, (effective_segment_number == segments), 0, 1);
    number_segments++;
    effective_segment_number++;
    if (effective_segment_number == (segments + 1))
      effective_segment_number = 1;
    double x_0 = tmpship.xpos;
    double y_0 = tmpship.ypos;
    tmpdist = sqrt(Distsq(x_0, y_0, x_1, y_1));
    switch (tmpship.whatdest) {
      case ScopeLevel::LEVEL_STAR:
        if (tmpdist <= (double)SYSTEMSIZE) trip_resolved = 1;
        break;
      case ScopeLevel::LEVEL_PLAN:
        if (tmpdist <= (double)PLORBITSIZE) trip_resolved = 1;
        break;
      case ScopeLevel::LEVEL_SHIP:
        if (tmpdist <= (double)DIST_TO_LAND) trip_resolved = 1;
        break;
      default:
        trip_resolved = 1;
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
