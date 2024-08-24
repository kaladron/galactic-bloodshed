// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fuel.c -- See estimations in fuel consumption and travel time. */

import gblib;
import std.compat;

#include "gb/fuel.h"

#include "gb/doship.h"

/**
 * @brief Outputs fuel information and estimated arrival time.
 *
 * This function outputs the total distance, number of segments, fuel amount,
 * and estimated arrival time based on the given parameters. It also handles
 * cases where the estimated arrival time is not available due to segment
 * discrepancy.
 *
 * @param g The GameObj reference.
 * @param dist The total distance.
 * @param fuel The amount of fuel.
 * @param grav The gravitational force.
 * @param mass The mass.
 * @param segs The number of segments.
 * @param plan_buf The plan buffer.
 */
void fuel_output(GameObj &g, const double dist, const double fuel,
                 const double grav, const double mass, const segments_t segs,
                 const std::string_view plan_buf) {
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
    return;
  }

  time_t effective_time =
      (segments == 1)
          ? next_update_time +
                (static_cast<time_t>((segs - 1) * (update_time * 60)))
          : next_segment_time + ((segs - 1) * (update_time / segments) * 60);

  g.out << std::format("ESTIMATED Arrival Time: {}\n",
                       std::ctime(&effective_time));
}

/**
 * @brief Performs a trip for a ship to a destination.
 *
 * This function calculates the number of segments required for a ship to reach
 * a destination. The ship's fuel, gravity factor, and starting coordinates are
 * used to determine the trip details.
 *
 * @param tmpdest The temporary destination place.
 * @param tmpship The ship to perform the trip.
 * @param fuel The amount of fuel available for the trip.
 * @param gravity_factor The gravity factor affecting the ship's movement.
 * @param x_1 The x-coordinate of the destination.
 * @param y_1 The y-coordinate of the destination.
 *
 * @return A tuple containing a boolean indicating if the trip was resolved
 * successfully and the number of segments taken.
 */
std::tuple<bool, segments_t> do_trip(const Place &tmpdest, Ship &tmpship,
                                     const double fuel,
                                     const double gravity_factor, double x_1,
                                     const double y_1) {
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
    moveship(tmpship, (effective_segment_number == segments), 0, 1);
    number_segments++;
    effective_segment_number++;
    if (effective_segment_number == (segments + 1))
      effective_segment_number = 1;
    double x_0 = tmpship.xpos;
    double y_0 = tmpship.ypos;
    double tmpdist = sqrt(Distsq(x_0, y_0, x_1, y_1));
    switch (tmpship.whatdest) {
      case ScopeLevel::LEVEL_STAR:
        if (tmpdist <= SYSTEMSIZE) trip_resolved = true;
        break;
      case ScopeLevel::LEVEL_PLAN:
        if (tmpdist <= PLORBITSIZE) trip_resolved = true;
        break;
      case ScopeLevel::LEVEL_SHIP:
        if (tmpdist <= DIST_TO_LAND) trip_resolved = true;
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
