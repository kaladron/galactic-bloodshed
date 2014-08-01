// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* makestar.c -- create, name, position, and make planets for a star. */

// G.O.D. [1] > methane melts at -182C
// G.O.D. [1] > it boils at -164
// G.O.D. [1] > ammonia melts at -78C
// G.O.D. [1] > boils at -33

#include "makestar.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "makeplanet.h"
#include "makeuniv.h"
#include "rand.h"
#include "tweakables.h"
#include "vars.h"

static const double PLANET_DIST_MAX = 1900.0;
static const double PLANET_DIST_MIN = 100.0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static char *NextStarName(void);
static const char *NextPlanetName(int);

static int Numtypes[TYPE_DESERT + 2] = {
    0,
};
static int Resource[TYPE_DESERT + 2] = {
    0,
};
static int Numsects[TYPE_DESERT + 2][PLATED + 1] = {
    {
     0,
    },
};
static int Fertsects[TYPE_DESERT + 2][PLATED + 1] = {
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

void PrintStatistics(void) {
  int i, j, y;

  printf("\nPlanet/Sector distribution -\n");
  printf("Type NP     .    *    ^    ~    #    (    -    NS   Avg     Res    "
         "Avg  A/Sec\n");
  for (i = 0; i <= TYPE_DESERT + 1; i++) {
    printf("%3.3s%4d ", Nametypes[i], Numtypes[i]);
    if (i < TYPE_DESERT + 1)
      Numtypes[TYPE_DESERT + 1] += Numtypes[i];
    for (j = 0; j < PLATED; j++) {
      printf("%5d", Numsects[i][j]);
      Numsects[i][PLATED] += Numsects[i][j];
      if (i <= TYPE_DESERT)
        Numsects[TYPE_DESERT + 1][j] += Numsects[i][j];
    }
    printf("%6d %5.1f", Numsects[i][PLATED],
           (1.0 * Numsects[i][PLATED]) / Numtypes[i]);
    printf("%8d %7.1f %5.1f\n", Resource[i],
           ((double)Resource[i]) / Numtypes[i],
           ((double)Resource[i]) / Numsects[i][PLATED]);
    Resource[TYPE_DESERT + 1] += Resource[i];
  }
  printf("Average Sector Fertility -\n");
  printf("Type NP     .    *    ^    ~    #    (    -    Fert  /Plan  /Sect\n");
  for (i = 0; i <= TYPE_DESERT + 1; i++) {
    printf("%3.3s%4d ", Nametypes[i], Numtypes[i]);
    y = 0;
    for (j = 0; j < PLATED; j++) {
      if (Numsects[i][j])
        printf("%5.1f", ((double)Fertsects[i][j]) / Numsects[i][j]);
      else
        printf("    -");
      y += Fertsects[i][j];
      Fertsects[TYPE_DESERT + 1][j] += Fertsects[i][j];
    }
    printf("%8d %7.1f %5.1f\n", y, (1.0 * y) / Numtypes[i],
           (1.0 * y) / Numsects[i][PLATED]);
  }
}

static int ReadNameList(char ss[1000][20], int n, int m, const char *filename) {
  int i, j;
  FILE *f = fopen(filename, "r");

  if (f == NULL) {
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
      if (EOF == j)
        goto out;
  next:
    ;
  }
out:
  fclose(f);
  printf("%d names listed in %s\n", i, filename);
  return i;
}

static void rand_list(int n, int *list) /* mix up the numbers 0 thru n */
{
  short nums[1000], i, j, k;
  short kk, ii;

  for (i = 0; i <= n; i++)
    nums[i] = 0;
  for (j = 0; j <= n; j++) {
    i = k = int_rand(0, n);
    while (nums[k] != 0)
      k += nums[k];
    list[j] = k;
    if (k == n)
      nums[k] = -n, kk = 0;
    else
      nums[k] = 1, kk = k + 1;
    /* K is now the next position in the list after the most recent number. */
    /* Go through the list, making each pointer point to k.  */
    while (i != k) {
      ii = i + nums[i];
      nums[i] = kk - i;
      i = ii;
    }
  }
}

void Makeplanet_init(void) {
  numplist = ReadNameList(PNames, 1000, 20, PLANETLIST);
  rand_list(numplist, planet_list);
  if (numplist < 0)
    exit(0);
  namepcount = 0;
}

static const char *NextPlanetName(int i) {
  const char *Numbers[] = {"1", "2",  "3",  "4",  "5",  "6",  "7", "8",
                           "9", "10", "11", "12", "13", "14", "15"};
  if (autoname_plan && (namepcount < numplist))
    return PNames[planet_list[namepcount++]];
  else
    return Numbers[i];
}

void Makestar_init(void) {
  numslist = ReadNameList(SNames, 1000, 20, STARLIST);
  rand_list(numslist, star_list);
  if (numslist < 0)
    exit(0);
  namestcount = 0;
}

static char *NextStarName(void) {
  static char buf[20];
  int i;

  if (autoname_star && (namestcount <= numslist))
    return SNames[star_list[namestcount++]];
  else {
    printf("Next star name:");
    for (i = 0; i < NAMESIZE - 4; i++)
      putchr('.');
    for (i = 0; i < NAMESIZE - 4; i++)
      putchr('\010'); /* ^H */
    scanf("%14[^\n]", buf);
    getchr();
  }
  return buf;
}

startype *Makestar(FILE *planetdata, FILE *sectordata) {
  planettype planet;
  int type, roll, temperature;
  int i, y, x;
  double dist, distmin, distmax, distsep;
  double angle, xpos, ypos;
  startype *Star;

  /* get names, positions of stars first */
  Star = (startype *)malloc(sizeof(startype));
  bzero(Star, sizeof(startype));
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
      type = TYPE_ASTEROID;
    } else if ((temperature > 100) && (temperature <= 400)) {
      if (roll <= 60)
        type = TYPE_MARS;
      else
        type = TYPE_DESERT;
    } else if ((temperature > 30) && (temperature <= 100)) {
      if (roll <= 25)
        type = TYPE_EARTH;
      else if (roll <= 50)
        type = TYPE_WATER;
      else if (roll <= 80)
        type = TYPE_FOREST;
      else if (roll <= 90)
        type = TYPE_DESERT;
      else
        type = TYPE_MARS;
    } else if ((temperature > -10) && (temperature <= 30)) {
      if (roll <= 45)
        type = TYPE_EARTH;
      else if (roll <= 70)
        type = TYPE_WATER;
      else if (roll <= 95)
        type = TYPE_FOREST;
      else
        type = TYPE_DESERT;
    } else if ((temperature > -50) && (temperature <= -10)) {
      if (roll <= 30)
        type = TYPE_DESERT;
      else if (roll <= 60)
        type = TYPE_ICEBALL;
      else if (roll <= 90)
        type = TYPE_FOREST;
      else
        type = TYPE_MARS;
    } else if ((temperature > -100) && (temperature <= -50)) {
      if (roll <= 50)
        type = TYPE_GASGIANT;
      else if (roll <= 80)
        type = TYPE_ICEBALL;
      else
        type = TYPE_MARS;
    } else if (temperature <= -100) {
      if (roll <= 80)
        type = TYPE_ICEBALL;
      else
        type = TYPE_GASGIANT;
    }
    planet = Makeplanet(dist, Star->temperature, type);
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
          switch (Sector(planet, x, y).condition) {
          case LAND:
            putchr(CHAR_LAND);
            break;
          case SEA:
            putchr(CHAR_SEA);
            break;
          case MOUNT:
            putchr(CHAR_MOUNT);
            break;
          case ICE:
            putchr(CHAR_ICE);
            break;
          case GAS:
            putchr(CHAR_GAS);
            break;
          case DESERT:
            putchr(CHAR_DESERT);
            break;
          case FOREST:
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
        uint8_t d = Sector(planet, x, y).condition;
        planet.total_resources += Sector(planet, x, y).resource;
        Resource[type] += Sector(planet, x, y).resource;
        Numsects[type][d]++;
        Fertsects[type][d] += Sector(planet, x, y).fert;
      }
    Star->planetpos[i] = (int)ftell(planetdata);
    /* posn of file-last write*/
    planet.sectormappos = (int)ftell(sectordata);       /* sector map pos */
    fwrite(&planet, sizeof(planettype), 1, planetdata); /* write planet */
    /* write each sector row */
    for (y = 0; y < planet.Maxy; y++)
      fwrite(&Sector(planet, 0, y), sizeof(sectortype), planet.Maxx,
             sectordata);
  }
  return Star;
}
