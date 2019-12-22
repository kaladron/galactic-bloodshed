// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fuel.c -- See estimations in fuel consumption and travel time. */

import gblib;
import std;

#include "gb/fuel.h"

#include "gb/GB_server.h"
#include "gb/doship.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/max.h"
#include "gb/moveship.h"
#include "gb/order.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static char plan_buf[1024];

static segments_t number_segments;
static double x_0, y_0, x_1, y_1;
static Ship *tmpship;

static int do_trip(const Place &, double fuel, double gravity_factor);
static void fuel_output(int Playernum, int Governor, double dist, double fuel,
                        double grav, double mass, segments_t segs);

void proj_fuel(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int opt_settings;
  int current_settings;
  int computing = 1;
  segments_t current_segs;
  double fuel_usage;
  double level;
  double dist;
  char buf[1024];
  double current_fuel = 0.0;
  double gravity_factor = 0.0;

  if ((argv.size() < 2) || (argv.size() > 3)) {
    notify(Playernum, Governor,
           "Invalid number of options.\n\"fuel "
           "#<shipnumber> [destination]\"...\n");
    return;
  }
  if (argv[1][0] != '#') {
    notify(Playernum, Governor,
           "Invalid first option.\n\"fuel #<shipnumber> [destination]\"...\n");
    return;
  }
  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno > Numships() || *shipno < 1) {
    sprintf(buf, "rst: no such ship #%lu \n", *shipno);
    notify(Playernum, Governor, buf);
    return;
  }
  auto ship = getship(*shipno);
  if (ship->owner != Playernum) {
    g.out << "You do not own this ship.\n";
    return;
  }
  if (landed(*ship) && (argv.size() == 2)) {
    g.out << "You must specify a destination for landed or docked ships...\n";
    return;
  }
  if (!ship->speed) {
    g.out << "That ship is not moving!\n";
    return;
  }
  if ((!speed_rating(*ship)) || (ship->type == ShipType::OTYPE_FACTORY)) {
    g.out << "That ship does not have a speed rating...\n";
    return;
  }
  if (landed(*ship) && (ship->whatorbits == ScopeLevel::LEVEL_PLAN)) {
    const auto p = getplanet(ship->storbits, ship->pnumorbits);
    gravity_factor = p.gravity();
    sprintf(plan_buf, "/%s/%s", Stars[(int)ship->storbits]->name,
            Stars[(int)ship->storbits]->pnames[(int)ship->pnumorbits]);
  }
  std::string deststr;
  if (argv.size() == 2) {
    deststr = prin_ship_dest(*ship);
  } else {
    deststr = argv[2];
  }
  Place tmpdest{g, deststr, true};
  if (tmpdest.err) {
    g.out << "fuel:  bad scope.\n";
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_SHIP) {
    (void)getship(&tmpship, tmpdest.shipno);
    if (!followable(&*ship, tmpship)) {
      g.out << "The ship's destination is out of range.\n";
      free(tmpship);
      return;
    }
    free(tmpship);
  }
  if (tmpdest.level != ScopeLevel::LEVEL_UNIV &&
      tmpdest.level != ScopeLevel::LEVEL_SHIP &&
      ((ship->storbits != tmpdest.snum) &&
       tmpdest.level != ScopeLevel::LEVEL_STAR) &&
      isclr(Stars[tmpdest.snum]->explored, ship->owner)) {
    g.out << "You haven't explored the destination system.\n";
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_UNIV) {
    g.out << "Invalid ship destination.\n";
    return;
  }
  x_0 = y_0 = x_1 = y_1 = 0.0;

  x_0 = ship->xpos;
  y_0 = ship->ypos;

  if (tmpdest.level == ScopeLevel::LEVEL_UNIV) {
    notify(Playernum, Governor,
           "That ship currently has no destination orders...\n");
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_SHIP) {
    (void)getship(&tmpship, tmpdest.shipno);
    if (tmpship->owner != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x_1 = tmpship->xpos;
    y_1 = tmpship->ypos;
    free(tmpship);
  } else if (tmpdest.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(tmpdest.snum, tmpdest.pnum);
    x_1 = p.xpos + Stars[tmpdest.snum]->xpos;
    y_1 = p.ypos + Stars[tmpdest.snum]->ypos;
  } else if (tmpdest.level == ScopeLevel::LEVEL_STAR) {
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
  (void)getship(&tmpship, *shipno);
  level = tmpship->fuel;
  current_settings = do_trip(tmpdest, tmpship->fuel, gravity_factor);
  current_segs = number_segments;
  if (current_settings) current_fuel = level - tmpship->fuel;
  free(tmpship);

  /*  2nd loop to determine lowest fuel needed...  */
  fuel_usage = level = tmpship->max_fuel;
  opt_settings = 0;
  while (computing) {
    (void)getship(&tmpship, *shipno);
    computing = do_trip(tmpdest, level, gravity_factor);
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

  (void)getship(&tmpship, *shipno);
  sprintf(buf,
          "\n  ----- ===== FUEL ESTIMATES ===== ----\n\nAt Current Fuel "
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

static int do_trip(const Place &tmpdest, double fuel, double gravity_factor) {
  segments_t effective_segment_number;
  int trip_resolved;
  double gravity_fuel;
  double tmpdist;
  double fuel_level1;

  tmpship->fuel = fuel; /* load up the pseudo-ship */
  effective_segment_number = nsegments_done;

  /*  Set our temporary destination.... */
  tmpship->destshipno = (unsigned short)tmpdest.shipno;
  tmpship->whatdest = tmpdest.level;
  tmpship->deststar = tmpdest.snum;
  tmpship->destpnum = tmpdest.pnum;
  if (tmpship->whatdest == ScopeLevel::LEVEL_SHIP || tmpship->ships) {
    /* Bring in the other ships.  moveship() uses ships[]. */
    Num_ships = Numships();
    ships = (Ship **)malloc(sizeof(Ship *) * (Num_ships) + 1);
    for (shipnum_t i = 1; i <= Num_ships; i++) (void)getship(&ships[i], i);
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
    moveship(tmpship, (effective_segment_number == segments), 0, 1);
    number_segments++;
    effective_segment_number++;
    if (effective_segment_number == (segments + 1))
      effective_segment_number = 1;
    x_0 = (double)tmpship->xpos;
    y_0 = (double)tmpship->ypos;
    tmpdist = sqrt(Distsq(x_0, y_0, x_1, y_1));
    switch (tmpship->whatdest) {
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
    if (((tmpship->fuel == fuel_level1) && (!tmpship->hyper_drive.on)) &&
        (trip_resolved == 0)) {
      if (tmpship->whatdest == ScopeLevel::LEVEL_SHIP) {
        for (shipnum_t i = 1; i <= Num_ships; i++) free(ships[i]);
        free(ships);
      }
      return (0);
    }
  }
  if (tmpship->whatdest == ScopeLevel::LEVEL_SHIP || tmpship->ships) {
    for (shipnum_t i = 1; i <= Num_ships; i++) free(ships[i]);
    free(ships);
  }
  return (1);
}
