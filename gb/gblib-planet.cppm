// SPDX-License-Identifier: Apache-2.0

export module gblib:planet;

import :race;
import :rand;
import :types;
import :tweakables;
import std;

export struct plinfo {     /* planetary stockpiles */
  unsigned short fuel;     /* fuel for powering things */
  unsigned short destruct; /* destructive potential */
  resource_t resource;     /* resources in storage */
  population_t popn;
  population_t troops;
  unsigned short crystals;

  unsigned short prod_res; /* shows last update production */
  unsigned short prod_fuel;
  unsigned short prod_dest;
  unsigned short prod_crystals;
  money_t prod_money;
  double prod_tech;

  money_t tech_invest;
  unsigned short numsectsowned;

  unsigned char comread;    /* combat readiness (mobilization)*/
  unsigned char mob_set;    /* mobilization target */
  unsigned char tox_thresh; /* min to build a waste can */

  unsigned char explored;
  unsigned char autorep;
  unsigned char tax;    /* tax rate */
  unsigned char newtax; /* new tax rate (after update) */
  unsigned char guns;   /* number of planet guns (mob/5) */

  /* merchant shipping parameters */
  struct {
    unsigned char set;         /* does the planet have orders? */
    unsigned char dest_star;   /* star that ship has to go to next */
    unsigned char dest_planet; /* planet destination */
    unsigned char load;        /* bit-field commodities to be loaded there */
    unsigned char unload;      /* unloaded commodities */
    unsigned char x, y;        /* location that ship has to land on */
  } route[MAX_ROUTES];         /* i am allowing up to four routes per planet */

  long mob_points;
  double est_production; /* estimated production */
};

export class Planet {
 public:
  Planet() = default;
  Planet(Planet &) = delete;
  Planet &operator=(const Planet &) = delete;
  Planet(Planet &&) = default;
  Planet &operator=(Planet &&) = default;
  ~Planet() {};

  double gravity() const;
  double compatibility(const Race &) const;
  ap_t get_points() const;

  double xpos, ypos;        /* x,y relative to orbit */
  shipnum_t ships;          /* first ship in orbit (to be changed) */
  unsigned char Maxx, Maxy; /* size of map */

  plinfo info[MAXPLAYERS];   /* player info */
  int conditions[TOXIC + 1]; /* atmospheric conditions for terraforming */

  population_t popn;
  population_t troops;
  population_t maxpopn; /* maximum population */
  resource_t total_resources;

  player_t slaved_to;
  PlanetType type;         /* what type planet is */
  unsigned char expltimer; /* timer for explorations */

  unsigned char explored;

  planetnum_t planet_id;
};

//* Return gravity for the Planet
double Planet::gravity() const {
  return (double)Maxx * (double)Maxy * GRAV_FACTOR;
}

double Planet::compatibility(const Race &race) const {
  double atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  int add = 0.1 * ((double)conditions[TEMP] - race.conditions[TEMP]);
  double sum = 1.0 - (double)abs(add) / 100.0;

  /* step through and report compatibility of each planetary gas */
  for (int i = TEMP + 1; i <= OTHER; i++) {
    add = (double)conditions[i] - race.conditions[i];
    atmosphere *= 1.0 - (double)abs(add) / 100.0;
  }
  sum *= atmosphere;
  sum *= 100.0 - conditions[TOXIC];

  if (sum < 0.0) return 0.0;
  return sum;
}

ap_t Planet::get_points() const {
  switch (type) {
    case PlanetType::ASTEROID:
      return ASTEROID_POINTS;
    case PlanetType::EARTH:
      return int_rand(EARTH_POINTS_LOW, EARTH_POINTS_HIGH);
    case PlanetType::MARS:
      return int_rand(MARS_POINTS_LOW, MARS_POINTS_HIGH);
    case PlanetType::ICEBALL:
      return int_rand(ICEBALL_POINTS_LOW, ICEBALL_POINTS_HIGH);
    case PlanetType::GASGIANT:
      return int_rand(GASGIANT_POINTS_LOW, GASGIANT_POINTS_HIGH);
    case PlanetType::WATER:
      return int_rand(WATER_POINTS_LOW, WATER_POINTS_HIGH);
    case PlanetType::FOREST:
      return int_rand(FOREST_POINTS_LOW, FOREST_POINTS_HIGH);
    case PlanetType::DESERT:
      return int_rand(DESERT_POINTS_LOW, DESERT_POINTS_HIGH);
  }
}

export int revolt(Planet &pl, const player_t victim, const player_t agent);

export void moveplanet(const starnum_t starnum, Planet &planet,
                       const planetnum_t planetnum);
export bool adjacent(const Planet &, int, int, int, int);
