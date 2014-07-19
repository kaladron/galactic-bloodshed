// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fuel.c -- See estimations in fuel consumption and travel time. */

#include "fuel.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "GB_server.h"
#include "doship.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "max.h"
#include "moveship.h"
#include "order.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

static char plan_buf[1024];

static int number_segments;
static double x_0, y_0, x_1, y_1;
static shiptype *tmpship;
static placetype tmpdest;

static int do_trip(double fuel, double gravity_factor);
static void fuel_output(int Playernum, int Governor, double dist, double fuel,
                        double grav, double mass, int segs);

void proj_fuel(int Playernum, int Governor, int APcount) {
  int shipno, opt_settings, current_settings, current_segs, computing = 1;
  double fuel_usage, level, dist;
  shiptype *ship;
  planettype *p;
  char buf[1024];
  double current_fuel = 0.0, gravity_factor = 0.0;

  if ((argn < 2) || (argn > 3)) {
    notify(Playernum, Governor, "Invalid number of options.\n\"fuel "
                                "#<shipnumber> [destination]\"...\n");
    return;
  }
  if (args[1][0] != '#') {
    notify(Playernum, Governor,
           "Invalid first option.\n\"fuel #<shipnumber> [destination]\"...\n");
    return;
  }
  sscanf(args[1] + (args[1][0] == '#'), "%d", &shipno);
  if (shipno > Numships() || shipno < 1) {
    sprintf(buf, "rst: no such ship #%d \n", shipno);
    notify(Playernum, Governor, buf);
    return;
  }
  (void)getship(&ship, shipno);
  if (ship->owner != Playernum) {
    notify(Playernum, Governor, "You do not own this ship.\n");
    free(ship);
    return;
  }
  if (landed(ship) && (argn == 2)) {
    notify(Playernum, Governor,
           "You must specify a destination for landed or docked ships...\n");
    free(ship);
    return;
  }
  if (!ship->speed) {
    notify(Playernum, Governor, "That ship is not moving!\n");
    free(ship);
    return;
  }
  if ((!speed_rating(ship)) || (ship->type == OTYPE_FACTORY)) {
    notify(Playernum, Governor, "That ship does not have a speed rating...\n");
    free(ship);
    return;
  }
  if (landed(ship) && (ship->whatorbits == LEVEL_PLAN)) {
    getplanet(&p, (int)ship->storbits, (int)ship->pnumorbits);
    gravity_factor = gravity(p);
    sprintf(plan_buf, "/%s/%s", Stars[(int)ship->storbits]->name,
            Stars[(int)ship->storbits]->pnames[(int)ship->pnumorbits]);
    free(p);
  }
  if (argn == 2)
    strcpy(args[2], prin_ship_dest(Playernum, Governor, ship));
  tmpdest = Getplace(Playernum, Governor, args[2], 1);
  if (tmpdest.err) {
    notify(Playernum, Governor, "fuel:  bad scope.\n");
    free(ship);
    return;
  }
  if (tmpdest.level == LEVEL_SHIP) {
    (void)getship(&tmpship, tmpdest.shipno);
    if (!followable(ship, tmpship)) {
      notify(Playernum, Governor, "The ship's destination is out of range.\n");
      free(tmpship);
      free(ship);
      return;
    }
    free(tmpship);
  }
  if (tmpdest.level != LEVEL_UNIV && tmpdest.level != LEVEL_SHIP &&
      ((ship->storbits != tmpdest.snum) && tmpdest.level != LEVEL_STAR) &&
      isclr(Stars[tmpdest.snum]->explored, ship->owner)) {
    notify(Playernum, Governor,
           "You haven't explored the destination system.\n");
    free(ship);
    return;
  }
  if (tmpdest.level == LEVEL_UNIV) {
    notify(Playernum, Governor, "Invalid ship destination.\n");
    free(ship);
    return;
  }
  x_0 = y_0 = x_1 = y_1 = 0.0;

  x_0 = ship->xpos;
  y_0 = ship->ypos;
  free(ship);

  if (tmpdest.level == LEVEL_UNIV) {
    notify(Playernum, Governor,
           "That ship currently has no destination orders...\n");
    return;
  }
  if (tmpdest.level == LEVEL_SHIP) {
    (void)getship(&tmpship, tmpdest.shipno);
    if (tmpship->owner != Playernum) {
      notify(Playernum, Governor, "Nice try.\n");
      return;
    }
    x_1 = tmpship->xpos;
    y_1 = tmpship->ypos;
    free(tmpship);
  } else if (tmpdest.level == LEVEL_PLAN) {
    getplanet(&p, (int)tmpdest.snum, (int)tmpdest.pnum);
    x_1 = p->xpos + Stars[tmpdest.snum]->xpos;
    y_1 = p->ypos + Stars[tmpdest.snum]->ypos;
    free(p);
  } else if (tmpdest.level == LEVEL_STAR) {
    x_1 = Stars[tmpdest.snum]->xpos;
    y_1 = Stars[tmpdest.snum]->ypos;
  } else
    printf("ERROR 99\n");

  /* compute the distance */
  dist = sqrt(Distsq(x_0, y_0, x_1, y_1));

  if (dist <= DIST_TO_LAND) {
    notify(Playernum, Governor,
           "That ship is within 10.0 units of the destination.\n");
    return;
  }

  /*  First get the results based on current fuel load.  */
  (void)getship(&tmpship, shipno);
  level = tmpship->fuel;
  current_settings = do_trip(tmpship->fuel, gravity_factor);
  current_segs = number_segments;
  if (current_settings)
    current_fuel = level - tmpship->fuel;
  free(tmpship);

  /*  2nd loop to determine lowest fuel needed...  */
  fuel_usage = level = tmpship->max_fuel;
  opt_settings = 0;
  while (computing) {
    (void)getship(&tmpship, shipno);
    computing = do_trip(level, gravity_factor);
    if ((computing) && (tmpship->fuel >= 0.05)) {
      fuel_usage = level;
      opt_settings = 1;
      level -= tmpship->fuel;
    } else if (computing) {
      computing = 0;
      fuel_usage = level;
    }
    free(tmpship);
  }

  (void)getship(&tmpship, shipno);
  sprintf(buf, "\n  ----- ===== FUEL ESTIMATES ===== ----\n\nAt Current Fuel "
               "Cargo (%.2ff):\n",
          tmpship->fuel);
  domass(tmpship);
  notify(Playernum, Governor, buf);
  if (!current_settings) {
    sprintf(buf, "The ship will not be able to complete the trip.\n");
    notify(Playernum, Governor, buf);
  } else
    fuel_output(Playernum, Governor, dist, current_fuel, gravity_factor,
                tmpship->mass, current_segs);
  sprintf(buf, "At Optimum Fuel Level (%.2ff):\n", fuel_usage);
  notify(Playernum, Governor, buf);
  if (!opt_settings) {
    sprintf(buf, "The ship will not be able to complete the trip.\n");
    notify(Playernum, Governor, buf);
  } else {
    tmpship->fuel = fuel_usage;
    domass(tmpship);
    fuel_output(Playernum, Governor, dist, fuel_usage, gravity_factor,
                tmpship->mass, number_segments);
  }
  free(tmpship);
}

static void fuel_output(int Playernum, int Governor, double dist, double fuel,
                        double grav, double mass, int segs) {
  char buf[1024], grav_buf[1024];

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
        next_segment_time + (long)((segs - 1) * (update_time / segments) * 60);
    if (segments == 1)
      effective_time =
          next_update_time + (long)((segs - 1) * (update_time * 60));
    sprintf(buf, "ESTIMATED Arrival Time: %s\n", ctime(&effective_time));
    notify(Playernum, Governor, buf);
    return;
  }
}

static int do_trip(double fuel, double gravity_factor) {
  segments_t effective_segment_number;
  int trip_resolved;
  double gravity_fuel, tmpdist, fuel_level1;

  tmpship->fuel = fuel; /* load up the pseudo-ship */
  effective_segment_number = nsegments_done;

  /*  Set our temporary destination.... */
  tmpship->destshipno = (unsigned short)tmpdest.shipno;
  tmpship->whatdest = tmpdest.level;
  tmpship->deststar = tmpdest.snum;
  tmpship->destpnum = tmpdest.pnum;
  if (tmpship->whatdest == LEVEL_SHIP || tmpship->ships) {
    /* Bring in the other ships.  Moveship() uses ships[]. */
    Num_ships = Numships();
    ships = (shiptype **)malloc(sizeof(shiptype *) * (Num_ships) + 1);
    for (shipnum_t i = 1; i <= Num_ships; i++)
      (void)getship(&ships[i], i);
  }

  trip_resolved = 0;
  number_segments = 0; /* Reset counter.  */

  /*  Launch the ship if it's on a planet.  */
  gravity_fuel = gravity_factor * tmpship->mass * LAUNCH_GRAV_MASS_FACTOR;
  tmpship->fuel -= gravity_fuel;
  tmpship->docked = 0;

  while (trip_resolved == 0) {
    domass(tmpship);
    fuel_level1 = tmpship->fuel;
    Moveship(tmpship, (effective_segment_number == segments), 0, 1);
    number_segments++;
    effective_segment_number++;
    if (effective_segment_number == (segments + 1))
      effective_segment_number = 1;
    x_0 = (double)tmpship->xpos;
    y_0 = (double)tmpship->ypos;
    tmpdist = sqrt(Distsq(x_0, y_0, x_1, y_1));
    switch ((int)tmpship->whatdest) {
    case LEVEL_STAR:
      if (tmpdist <= (double)SYSTEMSIZE)
        trip_resolved = 1;
      break;
    case LEVEL_PLAN:
      if (tmpdist <= (double)PLORBITSIZE)
        trip_resolved = 1;
      break;
    case LEVEL_SHIP:
      if (tmpdist <= (double)DIST_TO_LAND)
        trip_resolved = 1;
      break;
    default:
      trip_resolved = 1;
    }
    if (((tmpship->fuel == fuel_level1) && (!tmpship->hyper_drive.on)) &&
        (trip_resolved == 0)) {
      if (tmpship->whatdest == LEVEL_SHIP) {
        for (shipnum_t i = 1; i <= Num_ships; i++)
          free(ships[i]);
        free(ships);
      }
      return (0);
    }
  }
  if (tmpship->whatdest == LEVEL_SHIP || tmpship->ships) {
    for (shipnum_t i = 1; i <= Num_ships; i++)
      free(ships[i]);
    free(ships);
  }
  return (1);
}
