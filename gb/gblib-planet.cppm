// SPDX-License-Identifier: Apache-2.0

export module gblib:planet;

import :race;
import :rand;
import :types;
import :tweakables;
import std;

// Merchant shipping route parameters
export struct plroute {
  unsigned char set = 0;          // does the planet have orders?
  unsigned char dest_star = 0;    // star that ship has to go to next
  unsigned char dest_planet = 0;  // planet destination
  unsigned char load = 0;         // bit-field commodities to be loaded there
  unsigned char unload = 0;       // unloaded commodities
  unsigned char x = 0;            // location that ship has to land on
  unsigned char y = 0;
};

export struct plinfo {          // planetary stockpiles
  unsigned short fuel = 0;      // fuel for powering things
  unsigned short destruct = 0;  // destructive potential
  resource_t resource = 0;      // resources in storage
  population_t popn = 0;
  population_t troops = 0;
  unsigned short crystals = 0;

  unsigned short prod_res = 0;  // shows last update production
  unsigned short prod_fuel = 0;
  unsigned short prod_dest = 0;
  unsigned short prod_crystals = 0;
  money_t prod_money = 0;
  double prod_tech = 0;

  money_t tech_invest = 0;
  unsigned short numsectsowned = 0;

  unsigned char comread = 0;     // combat readiness (mobilization)
  unsigned char mob_set = 0;     // mobilization target
  unsigned char tox_thresh = 0;  // min to build a waste can

  unsigned char explored = 0;
  unsigned char autorep = 0;
  unsigned char tax = 0;     // tax rate
  unsigned char newtax = 0;  // new tax rate (after update)
  unsigned char guns = 0;    // number of planet guns (mob/5)

  /* merchant shipping parameters */
  plroute route[MAX_ROUTES];

  long mob_points = 0;
  double est_production = 0;  // estimated production
};

export class Planet {
 public:
  Planet() : type(PlanetType::EARTH) {};
  Planet(PlanetType type) : type(type) {};
  Planet(Planet &) = delete;
  Planet &operator=(const Planet &) = delete;
  Planet(Planet &&) = default;
  Planet &operator=(Planet &&) = default;
  ~Planet() {};

  [[nodiscard]] double gravity() const;
  [[nodiscard]] double compatibility(const Race &) const;
  [[nodiscard]] ap_t get_points() const;

  double xpos = 0;  // x,y relative to orbit */
  double ypos = 0;
  shipnum_t ships = 0;     // first ship in orbit (to be changed)
  unsigned char Maxx = 0;  // size of map
  unsigned char Maxy = 0;

  std::array<plinfo, MAXPLAYERS> info{};  // player info
  std::array<int, TOXIC + 1>
      conditions{};  // atmospheric conditions for terraforming

  population_t popn = 0;
  population_t troops = 0;
  population_t maxpopn = 0; /* maximum population */
  resource_t total_resources = 0;

  player_t slaved_to = 0;
  PlanetType type;             /* what type planet is */
  unsigned char expltimer = 0; /* timer for explorations */

  unsigned char explored = 0;

  starnum_t star_id = 0; /* star system containing this planet */
  planetnum_t planet_id = 0;
};

//* Return gravity for the Planet
double Planet::gravity() const {
  return (double)Maxx * (double)Maxy * GRAV_FACTOR;
}

double Planet::compatibility(const Race &race) const {
  double atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  int add = 0.1 * ((double)conditions[TEMP] - race.conditions[TEMP]);
  double sum = 1.0 - ((double)abs(add) / 100.0);

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

export int revolt(Planet &pl, player_t victim, player_t agent);

export void moveplanet(starnum_t starnum, Planet &planet,
                       planetnum_t planetnum);
export bool adjacent(const Planet &, Coordinates from, Coordinates to);
