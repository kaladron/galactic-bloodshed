// SPDX-License-Identifier: Apache-2.0

/// \file makestar.cc
/// \brief Create, name, position, and make planets for a star.
///
/// Atmospheric physics reference:
/// - Methane melts at -182C, boils at -164C
/// - Ammonia melts at -78C, boils at -33C

#include <sqlite3.h>
#include <cstdio>

import std;
import dallib;
import gblib;

#include "gb/creator/makeplanet.h"
#include "gb/creator/makestar.h"
#include "gb/creator/makeuniv.h"
#include "gb/files.h"

static const double PLANET_DIST_MAX = 1900.0;
static const double PLANET_DIST_MIN = 100.0;

static char* NextStarName();
static const char* NextPlanetName(int);

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
static std::array<std::array<char, 20>, 1000> planet_names;
static int planet_list[1000];
static int numslist, namestcount;
static std::array<std::array<char, 20>, 1000> star_names;
static int star_list[1000];

static int ReadNameList(std::array<std::array<char, 20>, 1000>& ss, int n,
                        int m, const char* filename);

// TODO(jeffbailey): This should be syncd with the ones in GB_server.h:
static const char* Nametypes[] = {"Earth",   "Asteroid", "Airless",
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
  printf("Type NP     .    *    ^    ~    #    (    -    NS   Avg     Res    "
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

static int ReadNameList(std::array<std::array<char, 20>, 1000>& ss, int n,
                        int m, const char* filename) {
  int i;
  int j;
  FILE* f = fopen(filename, "r");

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

void set_planet_list_permutation(const std::vector<int>& indices) {
  int copy_len = std::min<int>(indices.size(), 1000);
  for (int i = 0; i < copy_len; ++i) {
    planet_list[i] = indices[i];
  }
}

void set_star_list_permutation(const std::vector<int>& indices) {
  int copy_len = std::min<int>(indices.size(), 1000);
  for (int i = 0; i < copy_len; ++i) {
    star_list[i] = indices[i];
  }
}

void Makeplanet_init() {
  numplist = ReadNameList(planet_names, 1000, 20, PLANETLIST);
  if (numplist < 0) std::exit(0);
  auto shuffled = shuffled_indices(numplist + 1);
  set_planet_list_permutation(shuffled);
  namepcount = 0;
}

static const char* NextPlanetName(int i) {
  const char* Numbers[] = {"1", "2",  "3",  "4",  "5",  "6",  "7", "8",
                           "9", "10", "11", "12", "13", "14", "15"};
  if (autoname_plan && (namepcount < numplist))
    return planet_names[planet_list[namepcount++]].data();

  return Numbers[i];
}

void Makestar_init() {
  numslist = ReadNameList(star_names, 1000, 20, STARLIST);
  if (numslist < 0) std::exit(0);
  auto shuffled = shuffled_indices(numslist + 1);
  set_star_list_permutation(shuffled);
  namestcount = 0;
}

static char* NextStarName() {
  static char buf[20];
  int i;

  if (autoname_star && (namestcount <= numslist))
    return star_names[star_list[namestcount++]].data();

  printf("Next star name:");
  for (i = 0; i < NAMESIZE - 4; i++)
    std::putchar('.');
  for (i = 0; i < NAMESIZE - 4; i++)
    std::putchar('\010'); /* ^H */
  if (scanf("%14[^\n]", buf) < 0) {
    perror("Cannot read input");
    std::exit(-1);
  }
  std::getchar();

  return buf;
}

Star Makestar(Database& db, starnum_t snum) {
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
  star_struct star{};

  /* get names, positions of stars first */
  star.star_id = snum;
  star.gravity = int_rand(0, int_rand(0, 300)) + int_rand(0, 300) +
                 int_rand(100, 400) + int_rand(0, 9) / 10.0;
  star.temperature = round_rand(star.gravity / 100.0);
  /* + int_rand(0,2) - 1 ; */
  star.name = NextStarName();
  place_star(star);
  if (printstarinfo)
    printf("Star %s: gravity %1.1f, temp %d\n", star.name.c_str(), star.gravity,
           (int)star.temperature);
  /*
   * Generate planets for this star: */
  int num_planets = int_rand(minplanets, maxplanets);
  star.pnames.reserve(num_planets);  // Reserve space for efficiency

  distmin = PLANET_DIST_MIN;
  for (i = 0; i < num_planets; i++) {
    distsep = (PLANET_DIST_MAX - distmin) / (double)(num_planets - i);
    distmax = distmin + distsep;
    dist = distmin + double_rand() * (distmax - distmin);
    distmin = dist;

    temperature = Temperature(dist, star.temperature);
    angle = 2.0 * std::numbers::pi * double_rand();
    xpos = dist * std::sin(angle);
    ypos = dist * std::cos(angle);

    star.pnames.push_back(NextPlanetName(i));

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
    } else {
      throw std::runtime_error("No PlanetType left, bailing");
    }
    std::optional<SectorMap> smap_opt;
    auto planet = makeplanet(dist, star.temperature, type, snum, i, smap_opt);
    auto& smap = *smap_opt;

    planet.xpos() = xpos;
    planet.ypos() = ypos;
    planet.total_resources() = 0;
    Numtypes[type]++;
    if (printplaninfo) {
      printf("Planet %s: temp %d, type %s (%u)\n", star.pnames[i].c_str(),
             planet.conditions(RTEMP), Nametypes[planet.type()],
             static_cast<unsigned int>(planet.type()));
      printf("Position is (%1.0f,%1.0f) relative to %s; distance %1.0f.\n",
             planet.xpos(), planet.ypos(), star.name.c_str(), dist);
      printf("sect map(%dx%d):\n", planet.Maxx(), planet.Maxy());
      for (y = 0; y < planet.Maxy(); y++) {
        for (x = 0; x < planet.Maxx(); x++) {
          std::putchar(get_sector_char(smap.get({x, y}).get_condition()));
        }
        std::putchar('\n');
      }
      std::putchar('\n');
    }
    /*
     * Tabulate statistics for this star's planets. */
    for (y = 0; y < planet.Maxy(); y++)
      for (x = 0; x < planet.Maxx(); x++) {
        const auto& sect = smap.get({x, y});
        std::uint8_t d = sect.get_condition();
        planet.total_resources() += sect.get_resource();
        Resource[type] += sect.get_resource();
        Numsects[type][d]++;
        Fertsects[type][d] += sect.get_fert();
      }

    // Save sectormap and planet to database after calculations
    JsonStore store(db);
    SectorRepository(store).save_map(smap);
    PlanetRepository(store).save(planet);
  }
  return star;
}
