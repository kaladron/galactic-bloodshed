// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* makeplanet.c -- makes one planet. */

#include "makeplanet.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <tuple>
#include <vector>

#include "files_shl.h"
#include "makestar.h"
#include "rand.h"
#include "tweakables.h"
#include "vars.h"

/*             @   o   O   #   ~   .   (   -    */
static const int xmin[] = {15, 2, 4, 4, 26, 12, 12, 12};
static const int xmax[] = {23, 4, 8, 8, 32, 20, 20, 20};

/*
 * Fmin, Fmax, rmin, rmax are now all based on the sector type as well
 * as the planet type.
 */

/*                 .      *     ^     ~     #     (     -             */
static const int x_chance[] = {5, 15, 10, 4, 5, 7, 6};
static const int Fmin[][8] = {{25, 20, 10, 0, 20, 45, 5},  /*   @   */
                              {0, 1, 2, 0, 0, 0, 1},       /*   o   */
                              {0, 3, 2, 0, 0, 0, 2},       /*   O   */
                              {0, 0, 8, 0, 25, 0, 0},      /*   #   */
                              {0, 0, 0, 35, 0, 0, 0},      /*   ~   */
                              {30, 0, 0, 0, 20, 0, 0},     /*   .   */
                              {30, 25, 20, 0, 30, 60, 15}, /*   (   */
                              {0, 5, 2, 0, 0, 0, 2}};      /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int Fmax[][8] = {{40, 35, 20, 0, 40, 65, 15}, /*   @   */
                              {0, 2, 3, 0, 0, 0, 2},       /*   o   */
                              {0, 5, 4, 0, 0, 0, 3},       /*   O   */
                              {0, 0, 15, 0, 35, 0, 0},     /*   #   */
                              {0, 0, 0, 55, 0, 0, 0},      /*   ~   */
                              {50, 0, 0, 0, 40, 0, 0},     /*   .   */
                              {60, 45, 30, 0, 50, 90, 25}, /*   (   */
                              {0, 10, 6, 0, 0, 0, 8}};     /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int rmin[][8] = {{200, 225, 300, 0, 200, 0, 250}, /*   @   */
                              {0, 250, 350, 0, 0, 0, 300},     /*   o   */
                              {0, 225, 275, 0, 0, 0, 250},     /*   O   */
                              {0, 0, 250, 0, 225, 0, 0},       /*   #   */
                              {0, 0, 0, 30, 0, 0, 0},          /*   ~   */
                              {175, 0, 0, 0, 200, 0, 0},       /*   .   */
                              {150, 0, 0, 0, 150, 150, 0},     /*   (   */
                              {0, 200, 300, 0, 0, 0, 250}};    /*   -   */

/*                 .      *     ^     ~     #     (     -             */
static const int rmax[][8] = {{250, 325, 400, 0, 250, 0, 300}, /*   @   */
                              {0, 300, 600, 0, 0, 0, 400},     /*   o   */
                              {0, 300, 500, 0, 0, 0, 300},     /*   O   */
                              {0, 0, 350, 0, 300, 0, 0},       /*   #   */
                              {0, 0, 0, 60, 0, 0, 0},          /*   ~   */
                              {225, 0, 0, 0, 250, 0, 0},       /*   .   */
                              {250, 0, 0, 0, 250, 200, 0},     /*   (   */
                              {0, 200, 500, 0, 0, 0, 350}};    /*   -   */

/*  The starting conditions of the sectors given a planet types */
/*              @      o     O    #    ~    .       (       _  */
static const int cond[] = {SectorType::SEC_SEA,    SectorType::SEC_MOUNT,
                           SectorType::SEC_LAND,   SectorType::SEC_ICE,
                           SectorType::SEC_GAS,    SectorType::SEC_SEA,
                           SectorType::SEC_FOREST, SectorType::SEC_DESERT};

static int neighbors(sector_map &, int, int, int);
static void MakeEarthAtmosphere(Planet *, int);
static void Makesurface(const Planet &, sector_map &);
static short SectTemp(const Planet &, int);
static void seed(sector_map &, int, int);
static void grow(sector_map &, int, int, int);

Planet Makeplanet(double dist, short stemp, PlanetType type) {
  static planetnum_t planet_id = 0;
  int x, y;
  Planet planet;
  int atmos, total_sects;
  char c, t;
  double f;

  bzero(&planet, sizeof(planet));
  planet.planet_id = planet_id;
  planet_id++;
  planet.type = type;
  planet.expltimer = 5;
  planet.conditions[TEMP] = planet.conditions[RTEMP] = Temperature(dist, stemp);

  planet.Maxx = int_rand(xmin[type], xmax[type]);
  f = (double)planet.Maxx / RATIOXY;
  planet.Maxy = round_rand(f) + 1;
  if (!(planet.Maxy % 2)) planet.Maxy++; /* make odd number of latitude bands */

  if (type == PlanetType::ASTEROID)
    planet.Maxy = int_rand(1, 3); /* Asteroids have funny shapes. */

  t = c = cond[type];

  // Initialize with the correct number of sectors.
  sector_map smap(planet, true);
  for (y = 0; y < planet.Maxy; y++) {
    for (x = 0; x < planet.Maxx; x++) {
      auto &s = smap.get(x, y);
      s.type = s.condition = t;
    }
  }

  total_sects = (planet.Maxy - 1) * (planet.Maxx - 1);

  switch (type) {
    case PlanetType::GASGIANT: /* gas giant Planet */
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
    case PlanetType::MARS:
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
      seed(smap, SectorType::SEC_DESERT, int_rand(1, total_sects));
      seed(smap, SectorType::SEC_MOUNT, int_rand(1, total_sects));
      break;
    case PlanetType::ASTEROID: /* asteroid */
      /* no atmosphere */
      for (y = 0; y < planet.Maxy; y++)
        for (x = 0; x < planet.Maxx; x++)
          if (!int_rand(0, 3)) {
            auto &s = smap.get_random();
            s.type = s.condition = SectorType::SEC_LAND;
          }
      seed(smap, DESERT, int_rand(1, total_sects));
      break;
    case PlanetType::ICEBALL: /* ball of ice */
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
      seed(smap, SectorType::SEC_MOUNT, int_rand(1, total_sects / 2));
      break;
    case PlanetType::EARTH:
      MakeEarthAtmosphere(&planet, 33);
      seed(smap, SectorType::SEC_LAND,
           int_rand(total_sects / 30, total_sects / 20));
      grow(smap, SectorType::SEC_LAND, 1, 1);
      grow(smap, SectorType::SEC_LAND, 1, 2);
      grow(smap, SectorType::SEC_LAND, 2, 3);
      grow(smap, SectorType::SEC_SEA, 1, 4);
      break;
    case PlanetType::FOREST:
      MakeEarthAtmosphere(&planet, 0);
      seed(smap, SectorType::SEC_SEA,
           int_rand(total_sects / 30, total_sects / 20));
      grow(smap, SectorType::SEC_SEA, 1, 1);
      grow(smap, SectorType::SEC_SEA, 1, 3);
      grow(smap, SectorType::SEC_FOREST, 1, 3);
      break;
    case PlanetType::WATER:
      MakeEarthAtmosphere(&planet, 25);
      break;
    case PlanetType::DESERT:
      MakeEarthAtmosphere(&planet, 50);
      seed(smap, SectorType::SEC_MOUNT,
           int_rand(total_sects / 50, total_sects / 25));
      grow(smap, SectorType::SEC_MOUNT, 1, 1);
      grow(smap, SectorType::SEC_MOUNT, 1, 2);
      seed(smap, SectorType::SEC_LAND,
           int_rand(total_sects / 50, total_sects / 25));
      grow(smap, SectorType::SEC_LAND, 1, 1);
      grow(smap, SectorType::SEC_LAND, 1, 3);
      grow(smap, SectorType::SEC_DESERT, 1, 3);
      break;
  }
  Makesurface(planet,
              smap); /* determine surface geology based on environment */
  putsmap(smap, planet);
  return planet;
}

static void MakeEarthAtmosphere(Planet *pptr, int chance) {
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

static int neighbors(sector_map &smap, int x, int y, int type) {
  int l = x - 1;
  int r = x + 1; /* Left and right columns. */
  int n = 0;     /* Number of neighbors so far. */

  if (x == 0)
    l = smap.get_maxx() - 1;
  else if (r == smap.get_maxx())
    r = 0;
  if (y > 0)
    n += (smap.get(x, y - 1).type == type) + (smap.get(l, y - 1).type == type) +
         (smap.get(r, y - 1).type == type);

  n += (smap.get(l, y).type == type) + (smap.get(r, y).type == type);

  if (y < smap.get_maxy() - 1)
    n += (smap.get(x, y + 1).type == type) + (smap.get(l, y + 1).type == type) +
         (smap.get(r, y + 1).type == type);

  return (n);
}

//! Randomly places n sectors of designation type on a planet.
static void seed(sector_map &smap, int type, int n) {
  while (n-- > 0) {
    auto &s = smap.get_random();
    s.type = s.condition = type;
  }
}

/*! Spread out a sector of a certain type over the planet.  Rate is the number
 *  of adjacent sectors of the same type that must be found for the sector to
 *  become type.
 */
static void grow(sector_map &smap, int type, int n, int rate) {
  std::vector<std::tuple<int, int, int>> worklist;  // x, y, type

  // We don't want to alter the current map, as this is iterative.
  // So we store a worklist and apply it after we've done a scan of
  // the map.
  while (n-- > 0) {
    for (int x = 0; x < smap.get_maxx(); x++) {
      for (int y = 0; y < smap.get_maxy(); y++) {
        if (neighbors(smap, x, y, type) >= rate) {
          worklist.emplace_back(std::make_tuple(x, y, type));
        }
      }
    }
  }

  for (auto &item : worklist) {
    int x, y, type;
    std::tie(x, y, type) = item;
    auto &s = smap.get(x, y);
    s.condition = s.type = type;
  }
}

static void Makesurface(const Planet &p, sector_map &smap) {
  for (int x = 0; x < smap.get_maxx(); x++) {
    for (int y = 0; y < smap.get_maxy(); y++) {
      auto &s = smap.get(x, y);
      int temp = SectTemp(p, y);
      switch (s.type) {
        case SectorType::SEC_SEA:
          if (success(-temp) && ((y == 0) || (y == smap.get_maxy() - 1)))
            s.condition = SectorType::SEC_ICE;
          break;
        case SectorType::SEC_LAND:
          if (p.type == PlanetType::EARTH) {
            if (success(-temp) && (y == 0 || y == smap.get_maxy() - 1))
              s.condition = SectorType::SEC_ICE;
          }
          break;
        case SectorType::SEC_FOREST:
          if (p.type == PlanetType::FOREST) {
            if (success(-temp) && (y == 0 || y == smap.get_maxy() - 1))
              s.condition = SectorType::SEC_ICE;
          }
      }
      s.type = s.condition;
      s.resource = int_rand(rmin[p.type][s.type], rmax[p.type][s.type]);
      s.fert = int_rand(Fmin[p.type][s.type], Fmax[p.type][s.type]);
      if (int_rand(0, 1000) < x_chance[s.type])
        s.crystals = int_rand(4, 8);
      else
        s.crystals = 0;
    }
  }
}

static short SectTemp(const Planet &p, int y) {
  int dy, mid, temp;
  const int TFAC = 10;

  temp = p.conditions[TEMP];
  mid = (p.Maxy + 1) / 2 - 1;
  dy = abs(y - mid);

  temp -= TFAC * dy * dy;
  return temp;
}
