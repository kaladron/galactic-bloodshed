// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// // Use of this source code is governed by a license that can be
// // found in the COPYING file.

/* makeplanet.c -- makes one planet. */

#define EXTERN extern
#include "makeplanet.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "makestar.h"
#include "rand.h"
#include "tweakables.h"
#include "vars.h"

/*             @   o   O   #   ~   .   (   -    */
static const int xmin[] = { 15, 2, 4, 4, 26, 12, 12, 12 };
static const int xmax[] = { 23, 4, 8, 8, 32, 20, 20, 20 };

/*
 * Fmin, Fmax, rmin, rmax are now all based on the sector type as well
 * as the planet type.
 */

/*                 .      *     ^     ~     #     (     -             */
static const int x_chance[] = { 5, 15, 10, 4, 5, 7, 6 };
static const int Fmin[][8] = { { 25, 20, 10, 0, 20, 45, 5 },  /*   @   */
                               { 0, 1, 2, 0, 0, 0, 1 },       /*   o   */
                               { 0, 3, 2, 0, 0, 0, 2 },       /*   O   */
                               { 0, 0, 8, 0, 25, 0, 0 },      /*   #   */
                               { 0, 0, 0, 35, 0, 0, 0 },      /*   ~   */
                               { 30, 0, 0, 0, 20, 0, 0 },     /*   .   */
                               { 30, 25, 20, 0, 30, 60, 15 }, /*   (   */
                               { 0, 5, 2, 0, 0, 0, 2 } };     /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int Fmax[][8] = { { 40, 35, 20, 0, 40, 65, 15 }, /*   @   */
                               { 0, 2, 3, 0, 0, 0, 2 },       /*   o   */
                               { 0, 5, 4, 0, 0, 0, 3 },       /*   O   */
                               { 0, 0, 15, 0, 35, 0, 0 },     /*   #   */
                               { 0, 0, 0, 55, 0, 0, 0 },      /*   ~   */
                               { 50, 0, 0, 0, 40, 0, 0 },     /*   .   */
                               { 60, 45, 30, 0, 50, 90, 25 }, /*   (   */
                               { 0, 10, 6, 0, 0, 0, 8 } };    /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int rmin[][8] = { { 200, 225, 300, 0, 200, 0, 250 }, /*   @   */
                               { 0, 250, 350, 0, 0, 0, 300 },     /*   o   */
                               { 0, 225, 275, 0, 0, 0, 250 },     /*   O   */
                               { 0, 0, 250, 0, 225, 0, 0 },       /*   #   */
                               { 0, 0, 0, 30, 0, 0, 0 },          /*   ~   */
                               { 175, 0, 0, 0, 200, 0, 0 },       /*   .   */
                               { 150, 0, 0, 0, 150, 150, 0 },     /*   (   */
                               { 0, 200, 300, 0, 0, 0, 250 } };   /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int rmax[][8] = { { 250, 325, 400, 0, 250, 0, 300 }, /*   @   */
                               { 0, 300, 600, 0, 0, 0, 400 },     /*   o   */
                               { 0, 300, 500, 0, 0, 0, 300 },     /*   O   */
                               { 0, 0, 350, 0, 300, 0, 0 },       /*   #   */
                               { 0, 0, 0, 60, 0, 0, 0 },          /*   ~   */
                               { 225, 0, 0, 0, 250, 0, 0 },       /*   .   */
                               { 250, 0, 0, 0, 250, 200, 0 },     /*   (   */
                               { 0, 200, 500, 0, 0, 0, 350 } };   /*   -   */

/*  The starting conditions of the sectors given a planet types */
/*              @      o     O    #    ~    .       (       _  */
static const int cond[] = { SEA, MOUNT, LAND, ICE, GAS, SEA, FOREST, DESERT };

static int neighbors(planettype *, int, int, int);
static void MakeEarthAtmosphere(planettype *, int);
static void Makesurface(planettype *);
static short SectTemp(planettype *, int);
static void seed(planettype *, int, int);
static void grow(planettype *, int, int, int);

planettype Makeplanet(double dist, short stemp, int type) {
  reg int x, y;
  sectortype *s;
  planettype planet;
  int atmos, total_sects;
  char c, t;
  double f;

  Bzero(planet);
  bzero((char *)Smap, sizeof(Smap));
  planet.type = type;
  planet.expltimer = 5;
  planet.conditions[TEMP] = planet.conditions[RTEMP] = Temperature(dist, stemp);

  planet.Maxx = int_rand(xmin[type], xmax[type]);
  f = (double)planet.Maxx / RATIOXY;
  planet.Maxy = round_rand(f) + 1;
  if (!(planet.Maxy % 2))
    planet.Maxy++; /* make odd number of latitude bands */

  if (type == TYPE_ASTEROID)
    planet.Maxy = int_rand(1, 3); /* Asteroids have funny shapes. */

  t = c = cond[type];

  for (y = 0; y < planet.Maxy; y++) {
    for (x = 0; x < planet.Maxx; x++) {
      s = &Sector(planet, x, y);
      s->type = s->condition = t;
    }
  }

  total_sects = (planet.Maxy - 1) * (planet.Maxx - 1);

  switch (type) {
  case TYPE_GASGIANT: /* gas giant planet */
    /* either lots of meth or not too much */
    if (int_rand(0, 1)) { /* methane planet */
      atmos = 100 - (planet.conditions[METHANE] = int_rand(70, 80));
      atmos -= planet.conditions[HYDROGEN] = int_rand(1, atmos / 2);
      atmos -= planet.conditions[HELIUM] = 1;
      atmos -= planet.conditions[OXYGEN] = 0;
      atmos -= planet.conditions[CO2] = 1;
      atmos -= planet.conditions[NITROGEN] = int_rand(1, atmos / 2);
      atmos -= planet.conditions[SULFUR] = 0;
      planet.conditions[OTHER] = atmos;
    } else {
      atmos = 100 - (planet.conditions[HYDROGEN] = int_rand(30, 75));
      atmos -= planet.conditions[HELIUM] = int_rand(20, atmos / 2);
      atmos -= planet.conditions[METHANE] = random() & 01;
      atmos -= planet.conditions[OXYGEN] = 0;
      atmos -= planet.conditions[CO2] = random() & 01;
      atmos -= planet.conditions[NITROGEN] = int_rand(1, atmos / 2);
      atmos -= planet.conditions[SULFUR] = 0;
      planet.conditions[OTHER] = atmos;
    }
    break;
  case TYPE_MARS:
    planet.conditions[HYDROGEN] = 0;
    planet.conditions[HELIUM] = 0;
    planet.conditions[METHANE] = 0;
    planet.conditions[OXYGEN] = 0;
    if (random() & 01) { /* some have an atmosphere, some don't */
      atmos = 100 - (planet.conditions[CO2] = int_rand(30, 45));
      atmos -= planet.conditions[NITROGEN] = int_rand(10, atmos / 2);
      atmos -= planet.conditions[SULFUR] =
          (random() & 01) ? 0 : int_rand(20, atmos / 2);
      atmos -= planet.conditions[OTHER] = atmos;
    } else {
      planet.conditions[CO2] = 0;
      planet.conditions[NITROGEN] = 0;
      planet.conditions[SULFUR] = 0;
      planet.conditions[OTHER] = 0;
    }
    seed(&planet, DESERT, int_rand(1, total_sects));
    seed(&planet, MOUNT, int_rand(1, total_sects));
    break;
  case TYPE_ASTEROID: /* asteroid */
    /* no atmosphere */
    for (y = 0; y < planet.Maxy; y++)
      for (x = 0; x < planet.Maxx; x++)
        if (!int_rand(0, 3)) {
          s = &Sector(planet, int_rand(1, planet.Maxx),
                      int_rand(1, planet.Maxy));
          s->type = s->condition = LAND;
        }
    seed(&planet, DESERT, int_rand(1, total_sects));
    break;
  case TYPE_ICEBALL: /* ball of ice */
    /* no atmosphere */
    planet.conditions[HYDROGEN] = 0;
    planet.conditions[HELIUM] = 0;
    planet.conditions[METHANE] = 0;
    planet.conditions[OXYGEN] = 0;
    if (planet.Maxx * planet.Maxy > int_rand(0, 20)) {
      atmos = 100 - (planet.conditions[CO2] = int_rand(30, 45));
      atmos -= planet.conditions[NITROGEN] = int_rand(10, atmos / 2);
      atmos -= planet.conditions[SULFUR] =
          (random() & 01) ? 0 : int_rand(20, atmos / 2);
      atmos -= planet.conditions[OTHER] = atmos;
    } else {
      planet.conditions[CO2] = 0;
      planet.conditions[NITROGEN] = 0;
      planet.conditions[SULFUR] = 0;
      planet.conditions[OTHER] = 0;
    }
    seed(&planet, MOUNT, int_rand(1, total_sects / 2));
    break;
  case TYPE_EARTH:
    MakeEarthAtmosphere(&planet, 33);
    seed(&planet, LAND, int_rand(total_sects / 30, total_sects / 20));
    grow(&planet, LAND, 1, 1);
    grow(&planet, LAND, 1, 2);
    grow(&planet, LAND, 2, 3);
    grow(&planet, SEA, 1, 4);
    break;
  case TYPE_FOREST:
    MakeEarthAtmosphere(&planet, 0);
    seed(&planet, SEA, int_rand(total_sects / 30, total_sects / 20));
    grow(&planet, SEA, 1, 1);
    grow(&planet, SEA, 1, 3);
    grow(&planet, FOREST, 1, 3);
    break;
  case TYPE_WATER:
    MakeEarthAtmosphere(&planet, 25);
    break;
  case TYPE_DESERT:
    MakeEarthAtmosphere(&planet, 50);
    seed(&planet, MOUNT, int_rand(total_sects / 50, total_sects / 25));
    grow(&planet, MOUNT, 1, 1);
    grow(&planet, MOUNT, 1, 2);
    seed(&planet, LAND, int_rand(total_sects / 50, total_sects / 25));
    grow(&planet, LAND, 1, 1);
    grow(&planet, LAND, 1, 3);
    grow(&planet, DESERT, 1, 3);
    break;
  }
  Makesurface(&planet); /* determine surface geology based on environment */
  return planet;
}

static void MakeEarthAtmosphere(planettype *pptr, int chance) {
  int atmos = 100;

  if (int_rand(0, 99) > chance) {
    /* oxygen-reducing atmosphere */
    atmos -= pptr->conditions[OXYGEN] = int_rand(10, 25);
    atmos -= pptr->conditions[NITROGEN] = int_rand(20, atmos - 20);
    atmos -= pptr->conditions[CO2] = int_rand(10, atmos / 2);
    atmos -= pptr->conditions[HELIUM] = int_rand(2, atmos / 8 + 1);
    atmos -= pptr->conditions[METHANE] = random() & 01;
    atmos -= pptr->conditions[SULFUR] = 0;
    atmos -= pptr->conditions[HYDROGEN] = 0;
    pptr->conditions[OTHER] = atmos;
  } else {
    /* methane atmosphere */
    atmos -= pptr->conditions[METHANE] = int_rand(70, 80);
    atmos -= pptr->conditions[HYDROGEN] = int_rand(1, atmos / 2);
    atmos -= pptr->conditions[HELIUM] = 1 + (random() & 01);
    atmos -= pptr->conditions[OXYGEN] = 0;
    atmos -= pptr->conditions[CO2] = 1 + (random() & 01);
    atmos -= pptr->conditions[SULFUR] = (random() & 01);
    atmos -= pptr->conditions[NITROGEN] = int_rand(1, atmos / 2);
    pptr->conditions[OTHER] = atmos;
  }
}

/*
    Returns # of neighbors of a given designation that a sector has.
*/

static int neighbors(planettype *p, int x, int y, int type) {
  int l = x - 1;
  int r = x + 1; /* Left and right columns. */
  int n = 0;     /* Number of neighbors so far. */

  if (x == 0)
    l = p->Maxx - 1;
  else if (r == p->Maxx)
    r = 0;
  if (y > 0)
    n += (Sector(*p, x, y - 1).type == type) +
         (Sector(*p, l, y - 1).type == type) +
         (Sector(*p, r, y - 1).type == type);

  n += (Sector(*p, l, y).type == type) + (Sector(*p, r, y).type == type);

  if (y < p->Maxy - 1)
    n += (Sector(*p, x, y + 1).type == type) +
         (Sector(*p, l, y + 1).type == type) +
         (Sector(*p, r, y + 1).type == type);

  return (n);
}

/*
 * Randomly places n sectors of designation type on a planet.
 */

static void seed(planettype *p, int type, int n) {
  int x, y;
  sectortype *s;

  while (n-- > 0) {
    x = int_rand(0, p->Maxx - 1);
    y = int_rand(0, p->Maxy - 1);
    s = &Sector(*p, x, y);
    s->type = s->condition = type;
  }
}

/*
 * Spreat out a sector of a certain type over the planet.  Rate is the number
 * of adjacent sectors of the same type that must be found for the setor to
 * become type.
 */

static void grow(planettype *p, int type, int n, int rate) {
  int x, y;
  sectortype *s;
  sectortype Smap2[(MAX_X + 1) * (MAX_Y + 1) + 1];

  while (n-- > 0) {
    memcpy(Smap2, Smap, sizeof(Smap));
    for (x = 0; x < p->Maxx; x++) {
      for (y = 0; y < p->Maxy; y++) {
        if (neighbors(p, x, y, type) >= rate) {
          s = &Smap2[x + y * p->Maxx + 1];
          s->condition = s->type = type;
        }
      }
    }
    memcpy(Smap, Smap2, sizeof(Smap));
  }
}

static void Makesurface(planettype *p) {
  reg int x, y;
  reg int temp;
  reg sectortype *s;

  for (x = 0; x < p->Maxx; x++) {
    for (y = 0; y < p->Maxy; y++) {
      s = &Sector(*p, x, y);
      temp = SectTemp(p, y);
      switch (s->type) {
      case SEA:
        if (success(-temp) && ((y == 0) || (y == p->Maxy - 1)))
          s->condition = ICE;
        break;
      case LAND:
        if (p->type == TYPE_EARTH) {
          if (success(-temp) && (y == 0 || y == p->Maxy - 1))
            s->condition = ICE;
        }
        break;
      case FOREST:
        if (p->type == TYPE_FOREST) {
          if (success(-temp) && (y == 0 || y == p->Maxy - 1))
            s->condition = ICE;
        }
      }
      s->type = s->condition;
      s->resource = int_rand(rmin[p->type][s->type], rmax[p->type][s->type]);
      s->fert = int_rand(Fmin[p->type][s->type], Fmax[p->type][s->type]);
      if (int_rand(0, 1000) < x_chance[s->type])
        s->crystals = int_rand(4, 8);
      else
        s->crystals = 0;
    }
  }
}

static short SectTemp(planettype *p, int y) {
  register int dy, mid, temp;
  const int TFAC = 10;

  temp = p->conditions[TEMP];
  mid = (p->Maxy + 1) / 2 - 1;
  dy = abs(y - mid);

  temp -= TFAC * dy * dy;
  return temp;
  /*  return(p->conditions[TEMP]); */
}
