// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* makestar.c -- create, name, position, and make planets for a star. */

// G.O.D. [1] > methane melts at -182C
// G.O.D. [1] > it boils at -164
// G.O.D. [1] > ammonia melts at -78C
// G.O.D. [1] > boils at -33

#include "gb/creator/makestar.h"

import std;

#include "gb/creator/makeplanet.h"
#include "gb/creator/makeuniv.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static const double PLANET_DIST_MAX = 1900.0;
static const double PLANET_DIST_MIN = 100.0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static char *NextStarName();
static const char *NextPlanetName(int);

static int Numtypes[PlanetType::DESERT + 2] = {
    0,
};
static int Resource[PlanetType::DESERT + 2] = {
    0,
};
static int Numsects[PlanetType::DESERT + 2][SectorType::SEC_PLATED + 1] = {
    {
        0,
    },
};
static int Fertsects[PlanetType::DESERT + 2][SectorType::SEC_PLATED + 1] = {
    {
        0,
    },
};
static int numplist, namepcount;
static char PNames[1000][20];
static int planet_list[1000];
static int numslist, namestcount;
static char SNames[1000][20];
static int star_list[1000];

static int ReadNameList(char ss[1000][20], int n, int m, const char *filename);
static void rand_list(int n, int *list);

// TODO(jeffbailey): This should be syncd with the ones in GB_server.h:
static const char *Nametypes[] = {"Earth",   "Asteroid", "Airless",
                                  "Iceball", "Gaseous",  "Water",
                                  "Forest",  "Desert",   " >>"};

int Temperature(double dist, int stemp) {
  return -269 + stemp * 1315 * 40 / (40 + dist);
}

void PrintStatistics() {
  int i;
  int j;
  int y;

  printf("\nPlanet/Sector distribution -\n");
  printf(
      "Type NP     .    *    ^    ~    #    (    -    NS   Avg     Res    "
      "Avg  A/Sec\n");
  for (i = 0; i <= PlanetType::DESERT + 1; i++) {
    printf("%3.3s%4d ", Nametypes[i], Numtypes[i]);
    if (i < PlanetType::DESERT + 1)
      Numtypes[PlanetType::DESERT + 1] += Numtypes[i];
    for (j = 0; j < SectorType::SEC_PLATED; j++) {
      printf("%5d", Numsects[i][j]);
      Numsects[i][SectorType::SEC_PLATED] += Numsects[i][j];
      if (i <= PlanetType::DESERT)
        Numsects[PlanetType::DESERT + 1][j] += Numsects[i][j];
    }
    printf("%6d %5.1f", Numsects[i][SectorType::SEC_PLATED],
           (1.0 * Numsects[i][SectorType::SEC_PLATED]) / Numtypes[i]);
    printf("%8d %7.1f %5.1f\n", Resource[i],
           ((double)Resource[i]) / Numtypes[i],
           ((double)Resource[i]) / Numsects[i][SectorType::SEC_PLATED]);
    Resource[PlanetType::DESERT + 1] += Resource[i];
  }
  printf("Average Sector Fertility -\n");
  printf("Type NP     .    *    ^    ~    #    (    -    Fert  /Plan  /Sect\n");
  for (i = 0; i <= PlanetType::DESERT + 1; i++) {
    printf("%3.3s%4d ", Nametypes[i], Numtypes[i]);
    y = 0;
    for (j = 0; j < SectorType::SEC_PLATED; j++) {
      if (Numsects[i][j])
        printf("%5.1f", ((double)Fertsects[i][j]) / Numsects[i][j]);
      else
        printf("    -");
      y += Fertsects[i][j];
      Fertsects[PlanetType::DESERT + 1][j] += Fertsects[i][j];
    }
    printf("%8d %7.1f %5.1f\n", y, (1.0 * y) / Numtypes[i],
           (1.0 * y) / Numsects[i][SectorType::SEC_PLATED]);
  }
}

static int ReadNameList(char ss[1000][20], int n, int m, const char *filename) {
  int i;
  int j;
  FILE *f = fopen(filename, "r");

  if (f == nullptr) {
    printf("Unable to open \"%s\".\n", filename);
    return -1;
  }
  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++)
      if ('\n' == (ss[i][j] = getc(f))) {
        ss[i][j] = '\0';
        goto next;
      } else if (EOF == ss[i][j])
        goto out;
    ss[i][j - 1] = '\0';
    while ('\n' != (j = getc(f)))
      if (EOF == j) goto out;
  next:;
  }
out:
  fclose(f);
  printf("%d names listed in %s\n", i, filename);
  return i;
}

static void rand_list(int n, int *list) /* mix up the numbers 0 thru n */
{
  short nums[1000];
  short i;
  short j;
  short k;
  short kk;
  short ii;

  for (i = 0; i <= n; i++) nums[i] = 0;
  for (j = 0; j <= n; j++) {
    i = k = int_rand(0, n);
    while (nums[k] != 0) k += nums[k];
    list[j] = k;
    if (k == n) {
      nums[k] = -n;
      kk = 0;
    } else {
      nums[k] = 1;
      kk = k + 1;
    }
    /* K is now the next position in the list after the most recent number. */
    /* Go through the list, making each pointer point to k.  */
    while (i != k) {
      ii = i + nums[i];
      nums[i] = kk - i;
      i = ii;
    }
  }
}

void Makeplanet_init() {
  numplist = ReadNameList(PNames, 1000, 20, PLANETLIST);
  rand_list(numplist, planet_list);
  if (numplist < 0) exit(0);
  namepcount = 0;
}

static const char *NextPlanetName(int i) {
  const char *Numbers[] = {"1", "2",  "3",  "4",  "5",  "6",  "7", "8",
                           "9", "10", "11", "12", "13", "14", "15"};
  if (autoname_plan && (namepcount < numplist))
    return PNames[planet_list[namepcount++]];

  return Numbers[i];
}

void Makestar_init() {
  numslist = ReadNameList(SNames, 1000, 20, STARLIST);
  rand_list(numslist, star_list);
  if (numslist < 0) exit(0);
  namestcount = 0;
}

static char *NextStarName() {
  static char buf[20];
  int i;

  if (autoname_star && (namestcount <= numslist))
    return SNames[star_list[namestcount++]];

  printf("Next star name:");
  for (i = 0; i < NAMESIZE - 4; i++) putchr('.');
  for (i = 0; i < NAMESIZE - 4; i++) putchr('\010'); /* ^H */
  if (scanf("%14[^\n]", buf) < 0) {
    perror("Cannot read input");
    exit(-1);
  }
  getchr();

  return buf;
}

startype *Makestar(int snum) {
  PlanetType type;
  int roll;
  int temperature;
  int i;
  int y;
  int x;
  double dist;
  double distmin;
  double distmax;
  double distsep;
  double angle;
  double xpos;
  double ypos;
  startype *Star;

  /* get names, positions of stars first */
  Star = (startype *)malloc(sizeof(startype));
  bzero(Star, sizeof(startype));
  Star->star_id = snum;
  Star->gravity = int_rand(0, int_rand(0, 300)) + int_rand(0, 300) +
                  int_rand(100, 400) + int_rand(0, 9) / 10.0;
  Star->temperature = round_rand(Star->gravity / 100.0);
  /* + int_rand(0,2) - 1 ; */
  strcpy(Star->name, NextStarName());
  place_star(Star);
  if (printstarinfo)
    printf("Star %s: gravity %1.1f, temp %d\n", Star->name, Star->gravity,
           (int)Star->temperature);
  /*
   * Generate planets for this star: */
  Star->numplanets = int_rand(minplanets, maxplanets);

  distmin = PLANET_DIST_MIN;
  for (i = 0; i < Star->numplanets; i++) {
    distsep = (PLANET_DIST_MAX - distmin) / (double)(Star->numplanets - i);
    distmax = distmin + distsep;
    dist = distmin + double_rand() * (distmax - distmin);
    distmin = dist;

    temperature = Temperature(dist, Star->temperature);
    angle = 2.0 * M_PI * double_rand();
    xpos = dist * sin(angle);
    ypos = dist * cos(angle);

    strcpy(Star->pnames[i], NextPlanetName(i));

    roll = int_rand(1, 100);
    if ((int_rand(1, 100) <= 10) || (temperature > 400)) {
      type = PlanetType::ASTEROID;
    } else if ((temperature > 100) && (temperature <= 400)) {
      if (roll <= 60)
        type = PlanetType::MARS;
      else
        type = PlanetType::DESERT;
    } else if ((temperature > 30) && (temperature <= 100)) {
      if (roll <= 25)
        type = PlanetType::EARTH;
      else if (roll <= 50)
        type = PlanetType::WATER;
      else if (roll <= 80)
        type = PlanetType::FOREST;
      else if (roll <= 90)
        type = PlanetType::DESERT;
      else
        type = PlanetType::MARS;
    } else if ((temperature > -10) && (temperature <= 30)) {
      if (roll <= 45)
        type = PlanetType::EARTH;
      else if (roll <= 70)
        type = PlanetType::WATER;
      else if (roll <= 95)
        type = PlanetType::FOREST;
      else
        type = PlanetType::DESERT;
    } else if ((temperature > -50) && (temperature <= -10)) {
      if (roll <= 30)
        type = PlanetType::DESERT;
      else if (roll <= 60)
        type = PlanetType::ICEBALL;
      else if (roll <= 90)
        type = PlanetType::FOREST;
      else
        type = PlanetType::MARS;
    } else if ((temperature > -100) && (temperature <= -50)) {
      if (roll <= 50)
        type = PlanetType::GASGIANT;
      else if (roll <= 80)
        type = PlanetType::ICEBALL;
      else
        type = PlanetType::MARS;
    } else if (temperature <= -100) {
      if (roll <= 80)
        type = PlanetType::ICEBALL;
      else
        type = PlanetType::GASGIANT;
    }
    auto planet = Makeplanet(dist, Star->temperature, type);
    auto smap = getsmap(planet);
    planet.xpos = xpos;
    planet.ypos = ypos;
    planet.total_resources = 0;
    Numtypes[type]++;
    if (printplaninfo) {
      printf("Planet %s: temp %d, type %s (%u)\n", Star->pnames[i],
             planet.conditions[RTEMP], Nametypes[planet.type], planet.type);
      printf("Position is (%1.0f,%1.0f) relative to %s; distance %1.0f.\n",
             planet.xpos, planet.ypos, Star->name, dist);
      printf("sect map(%dx%d):\n", planet.Maxx, planet.Maxy);
      for (y = 0; y < planet.Maxy; y++) {
        for (x = 0; x < planet.Maxx; x++) {
          switch (smap.get(x, y).condition) {
            case SectorType::SEC_LAND:
              putchr(CHAR_LAND);
              break;
            case SectorType::SEC_SEA:
              putchr(CHAR_SEA);
              break;
            case SectorType::SEC_MOUNT:
              putchr(CHAR_MOUNT);
              break;
            case SectorType::SEC_ICE:
              putchr(CHAR_ICE);
              break;
            case SectorType::SEC_GAS:
              putchr(CHAR_GAS);
              break;
            case SectorType::SEC_DESERT:
              putchr(CHAR_DESERT);
              break;
            case SectorType::SEC_FOREST:
              putchr(CHAR_FOREST);
              break;
            default:
              putchr('?');
              break;
          }
        }
        putchr('\n');
      }
      putchr('\n');
    }
    /*
     * Tabulate statistics for this star's planets. */
    for (y = 0; y < planet.Maxy; y++)
      for (x = 0; x < planet.Maxx; x++) {
        uint8_t d = smap.get(x, y).condition;
        planet.total_resources += smap.get(x, y).resource;
        Resource[type] += smap.get(x, y).resource;
        Numsects[type][d]++;
        Fertsects[type][d] += smap.get(x, y).fert;
      }
    Star->planetpos[i] = 0;  // old posn of file-last write
    putplanet(planet, Star, i);
  }
  return Star;
}
