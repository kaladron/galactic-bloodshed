// SPDX-License-Identifier: Apache-2.0

/// \file makeplanet.cc
/// \brief Generates individual planets for star systems.

module;

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import std;

module gb.creator;

namespace GB::creator {

constexpr PlanetValues<int> xmin = {
    .earth = 15,
    .asteroid = 2,
    .mars = 4,
    .iceball = 4,
    .gasgiant = 26,
    .water = 12,
    .forest = 12,
    .desert = 12,
};

constexpr PlanetValues<int> xmax = {
    .earth = 23,
    .asteroid = 4,
    .mars = 8,
    .iceball = 8,
    .gasgiant = 32,
    .water = 20,
    .forest = 20,
    .desert = 20,
};

/*
 * Fmin, Fmax, rmin, rmax are now all based on the sector type as well
 * as the planet type (omitted terrain fields default to 0).
 */
constexpr SectorValues<int> x_chance = {
    .sea = 5,
    .land = 15,
    .mount = 10,
    .gas = 4,
    .ice = 5,
    .forest = 7,
    .desert = 6,
};

constexpr PlanetValues<SectorValues<int>> Fmin = {
    .earth = {.sea = 25,
              .land = 20,
              .mount = 10,
              .ice = 20,
              .forest = 45,
              .desert = 5},
    .asteroid = {.land = 1, .mount = 2, .desert = 1},
    .mars = {.land = 3, .mount = 2, .desert = 2},
    .iceball = {.mount = 8, .ice = 25},
    .gasgiant = {.gas = 35},
    .water = {.sea = 30, .ice = 20},
    .forest = {.sea = 30,
               .land = 25,
               .mount = 20,
               .ice = 30,
               .forest = 60,
               .desert = 15},
    .desert = {.land = 5, .mount = 2, .desert = 2},
};

constexpr PlanetValues<SectorValues<int>> Fmax = {
    .earth = {.sea = 40,
              .land = 35,
              .mount = 20,
              .ice = 40,
              .forest = 65,
              .desert = 15},
    .asteroid = {.land = 2, .mount = 3, .desert = 2},
    .mars = {.land = 5, .mount = 4, .desert = 3},
    .iceball = {.mount = 15, .ice = 35},
    .gasgiant = {.gas = 55},
    .water = {.sea = 50, .ice = 40},
    .forest = {.sea = 60,
               .land = 45,
               .mount = 30,
               .ice = 50,
               .forest = 90,
               .desert = 25},
    .desert = {.land = 10, .mount = 6, .desert = 8},
};

constexpr PlanetValues<SectorValues<int>> rmin = {
    .earth = {.sea = 200, .land = 225, .mount = 300, .ice = 200, .desert = 250},
    .asteroid = {.land = 250, .mount = 350, .desert = 300},
    .mars = {.land = 225, .mount = 275, .desert = 250},
    .iceball = {.mount = 250, .ice = 225},
    .gasgiant = {.gas = 30},
    .water = {.sea = 175, .ice = 200},
    .forest = {.sea = 150, .ice = 150, .forest = 150},
    .desert = {.land = 200, .mount = 300, .desert = 250},
};

constexpr PlanetValues<SectorValues<int>> rmax = {
    .earth = {.sea = 250, .land = 325, .mount = 400, .ice = 250, .desert = 300},
    .asteroid = {.land = 300, .mount = 600, .desert = 400},
    .mars = {.land = 300, .mount = 500, .desert = 300},
    .iceball = {.mount = 350, .ice = 300},
    .gasgiant = {.gas = 60},
    .water = {.sea = 225, .ice = 250},
    .forest = {.sea = 250, .ice = 250, .forest = 200},
    .desert = {.land = 200, .mount = 500, .desert = 350},
};

/* The starting conditions of the sectors given a planet type */
constexpr PlanetValues<SectorType> cond = {
    .earth = SectorType::SEC_SEA,
    .asteroid = SectorType::SEC_MOUNT,
    .mars = SectorType::SEC_LAND,
    .iceball = SectorType::SEC_ICE,
    .gasgiant = SectorType::SEC_GAS,
    .water = SectorType::SEC_SEA,
    .forest = SectorType::SEC_FOREST,
    .desert = SectorType::SEC_DESERT,
};

namespace {
void MakeEarthAtmosphere(Planet& planet, const int chance) {
  int atmos = 100;

  if (uniform_rand(0, 99) > chance) {
    /* oxygen-reducing atmosphere */
    atmos -= (planet.conditions().oxygen = uniform_rand(10, 25));
    atmos -= (planet.conditions().nitrogen = uniform_rand(20, atmos - 20));
    atmos -= (planet.conditions().co2 = uniform_rand(10, atmos / 2));
    atmos -= (planet.conditions().helium = uniform_rand(2, (atmos / 8) + 1));
    atmos -= (planet.conditions().methane = uniform_rand(0, 1));
    atmos -= (planet.conditions().sulfur = 0);
    atmos -= (planet.conditions().hydrogen = 0);
    planet.conditions().other = atmos;
  } else {
    /* methane atmosphere */
    atmos -= (planet.conditions().methane = uniform_rand(70, 80));
    atmos -= (planet.conditions().hydrogen = uniform_rand(1, atmos / 2));
    atmos -= (planet.conditions().helium = 1 + uniform_rand(0, 1));
    atmos -= (planet.conditions().oxygen = 0);
    atmos -= (planet.conditions().co2 = 1 + uniform_rand(0, 1));
    atmos -= (planet.conditions().sulfur = uniform_rand(0, 1));
    atmos -= (planet.conditions().nitrogen = uniform_rand(1, atmos / 2));
    planet.conditions().other = atmos;
  }
}

//! Returns # of neighbors of a given designation that a sector has.
int neighbors(const Planet& planet, const SectorMap& smap, Coordinates coords,
              SectorType type) {
  return static_cast<int>(std::ranges::count_if(
      smap.adjacent(planet, coords),
      [type](const Sector& s) noexcept { return s.get_type() == type; }));
}

//! Randomly places n sectors of designation type on a planet.
void seed(SectorMap& smap, SectorType type, int n) {
  while (n-- > 0) {
    auto& s = smap.get_random();
    s.set_type(type);
    s.set_condition(type);
  }
}

/*! Spread out a sector of a certain type over the planet.  Rate is the number
 *  of adjacent sectors of the same type that must be found for the sector to
 *  become type.
 */
void grow(const Planet& planet, SectorMap& smap, SectorType type, int n,
          int rate) {
  std::vector<Coordinates> worklist;
  while (n-- > 0) {
    worklist.clear();
    for (const Coordinates c : smap.coordinates()) {
      if (neighbors(planet, smap, c, type) >= rate) {
        worklist.push_back(c);
      }
    }
    for (const Coordinates c : worklist) {
      auto& s = smap.get(c);
      s.set_condition(type);
      s.set_type(type);
    }
  }
}

/**
 * @brief Calculates the temperature of a sector on a planet based on its
 * latitude.
 *
 * This function computes the temperature for a specific sector at the
 * y-coordinate `y` on the planet `p`. The temperature decreases quadratically
 * with the distance from the planet's equator.
 *
 * @param p The planet object containing planetary conditions and dimensions.
 * @param y The y-coordinate (latitude index) of the sector.
 * @return The calculated temperature of the sector.
 */
int SectTemp(const Planet& p, const int y) {
  // Temperature factor.
  const int TFAC = 10;

  int temp = p.temp();
  int mid = ((p.dimensions().y + 1) / 2) - 1;
  int dy = std::abs(y - mid);

  temp -= TFAC * dy * dy;
  return temp;
}

/**
 * @brief Converts polar sectors to ice based on temperature and planet type.
 *
 * @param p The planet containing planetary conditions.
 * @param s The polar sector to potentially freeze.
 * @param maxy The planet's latitude dimension (height).
 */
void maybe_ice_pole(const Planet& p, Sector& s, int maxy) {
  if (s.coords().y != 0 && s.coords().y != maxy - 1) {
    return;
  }
  const int temp = SectTemp(p, s.coords().y);
  switch (s.get_condition()) {
    case SectorType::SEC_SEA:
      if (success(-temp)) {
        s.set_condition(SectorType::SEC_ICE);
      }
      break;
    case SectorType::SEC_LAND:
      if (p.type() == PlanetType::EARTH && success(-temp)) {
        s.set_condition(SectorType::SEC_ICE);
      }
      break;
    case SectorType::SEC_FOREST:
      if (p.type() == PlanetType::FOREST && success(-temp)) {
        s.set_condition(SectorType::SEC_ICE);
      }
      break;
    default:
      break;
  }
}

/**
 * @brief Generates the surface of a planet by initializing each sector's
 * properties.
 *
 * This function iterates over all sectors in the given `SectorMap` and assigns
 * initial values to each sector's type, resources, fertility, and crystals. It
 * also applies special conditions to polar sectors, potentially converting them
 * to ice based on temperature and planet type.
 *
 * @param p The planet for which the surface is being generated.
 * @param smap The sector map representing the planet's surface.
 */
void Makesurface(const Planet& p, SectorMap& smap) {
  const int maxy = smap.dimensions().y;
  for (auto& s : smap) {
    maybe_ice_pole(p, s, maxy);
    s.set_type(s.get_condition());
    s.set_resource(uniform_rand(rmin[p.type()][s.get_type()],
                                rmax[p.type()][s.get_type()]));
    s.set_fert(uniform_rand(Fmin[p.type()][s.get_type()],
                            Fmax[p.type()][s.get_type()]));

    if (uniform_rand(0, 1000) < x_chance[s.get_type()]) {
      s.set_crystals(uniform_rand(4, 8));
    }
  }
}
}  // namespace

Planet makeplanet(double dist, short stemp, PlanetType type, starnum_t star_id,
                  planetnum_t planet_order,
                  std::optional<SectorMap>& out_smap) {
  int maxx = uniform_rand(xmin[type], xmax[type]);
  auto f = (double)maxx / RATIOXY;
  int maxy = round_rand(f) + 1;
  if (!(maxy % 2)) maxy++; /* make odd number of latitude bands */

  if (type == PlanetType::ASTEROID)
    maxy = uniform_rand(1, 3); /* Asteroids have funny shapes. */

  Planet planet{star_id, planet_order, type, Coordinates{maxx, maxy}};
  planet.expltimer() = 5;
  planet.temp() = planet.rtemp() = calculate_temperature(dist, stemp);

  auto t = cond[type];

  // Initialize with the correct number of sectors.
  SectorMap smap(planet);
  for (auto& s : smap) {
    s.set_type(t);
    s.set_condition(t);
  }

  const int total_sects = planet.num_sectors();

  switch (type) {
    case PlanetType::GASGIANT: /* gas giant Planet */
      /* either lots of meth or not too much */
      if (uniform_rand(0, 1)) { /* methane planet */
        auto atmos = 100 - (planet.conditions().methane = uniform_rand(70, 80));
        atmos -= (planet.conditions().hydrogen = uniform_rand(1, atmos / 2));
        atmos -= (planet.conditions().helium = 1);
        atmos -= (planet.conditions().oxygen = 0);
        atmos -= (planet.conditions().co2 = 1);
        atmos -= (planet.conditions().nitrogen = uniform_rand(1, atmos / 2));
        atmos -= (planet.conditions().sulfur = 0);
        planet.conditions().other = atmos;
      } else {
        auto atmos =
            100 - (planet.conditions().hydrogen = uniform_rand(30, 75));
        atmos -= (planet.conditions().helium = uniform_rand(20, atmos / 2));
        atmos -= (planet.conditions().methane = uniform_rand(0, 1));
        atmos -= (planet.conditions().oxygen = 0);
        atmos -= (planet.conditions().co2 = uniform_rand(0, 1));
        atmos -= (planet.conditions().nitrogen = uniform_rand(1, atmos / 2));
        atmos -= (planet.conditions().sulfur = 0);
        planet.conditions().other = atmos;
      }
      break;
    case PlanetType::MARS:
      planet.conditions().hydrogen = 0;
      planet.conditions().helium = 0;
      planet.conditions().methane = 0;
      planet.conditions().oxygen = 0;
      if (uniform_rand(0, 1)) { /* some have an atmosphere, some don't */
        auto atmos = 100 - (planet.conditions().co2 = uniform_rand(30, 45));
        atmos -= (planet.conditions().nitrogen = uniform_rand(10, atmos / 2));
        atmos -= (planet.conditions().sulfur =
                      uniform_rand(0, 1) ? 0 : uniform_rand(20, atmos / 2));
        planet.conditions().other = atmos;
      } else {
        planet.conditions().co2 = 0;
        planet.conditions().nitrogen = 0;
        planet.conditions().sulfur = 0;
        planet.conditions().other = 0;
      }
      seed(smap, SectorType::SEC_DESERT, uniform_rand(1, total_sects));
      seed(smap, SectorType::SEC_MOUNT, uniform_rand(1, total_sects));
      break;
    case PlanetType::ASTEROID: /* asteroid */
      /* no atmosphere */
      for (auto& s : smap) {
        if (!uniform_rand(0, 3)) {
          s.set_type(SectorType::SEC_LAND);
          s.set_condition(SectorType::SEC_LAND);
        }
      }
      seed(smap, SectorType::SEC_DESERT, uniform_rand(1, total_sects));
      break;
    case PlanetType::ICEBALL: /* ball of ice */
      /* Base atmospheric conditions */
      planet.conditions().hydrogen = 0;
      planet.conditions().helium = 0;
      planet.conditions().methane = 0;
      planet.conditions().oxygen = 0;

      // Atmospheric retention algorithm:
      // Larger iceball bodies (sufficient surface area and gravity) can hold
      // a trace atmosphere composed of CO2, Nitrogen, Sulfur, and Other gases.
      // Bodies with > 20 sectors are guaranteed to retain an atmosphere, while
      // smaller bodies have a probability proportional to their sector count.
      if (planet.num_sectors() > uniform_rand(0, 20)) {
        auto atmos = 100 - (planet.conditions().co2 = uniform_rand(30, 45));
        atmos -= (planet.conditions().nitrogen = uniform_rand(10, atmos / 2));
        atmos -= (planet.conditions().sulfur =
                      uniform_rand(0, 1) ? 0 : uniform_rand(20, atmos / 2));
        planet.conditions().other = atmos;
      } else {
        planet.conditions().co2 = 0;
        planet.conditions().nitrogen = 0;
        planet.conditions().sulfur = 0;
        planet.conditions().other = 0;
      }
      seed(smap, SectorType::SEC_MOUNT,
           uniform_rand(1, std::max(1, total_sects / 2)));
      break;
    case PlanetType::EARTH:
      MakeEarthAtmosphere(planet, 33);
      seed(smap, SectorType::SEC_LAND,
           uniform_rand(total_sects / 30, total_sects / 20));
      grow(planet, smap, SectorType::SEC_LAND, 1, 1);
      grow(planet, smap, SectorType::SEC_LAND, 1, 2);
      grow(planet, smap, SectorType::SEC_LAND, 2, 3);
      grow(planet, smap, SectorType::SEC_SEA, 1, 4);
      break;
    case PlanetType::FOREST:
      MakeEarthAtmosphere(planet, 0);
      seed(smap, SectorType::SEC_SEA,
           uniform_rand(total_sects / 30, total_sects / 20));
      grow(planet, smap, SectorType::SEC_SEA, 1, 1);
      grow(planet, smap, SectorType::SEC_SEA, 1, 3);
      grow(planet, smap, SectorType::SEC_FOREST, 1, 3);
      break;
    case PlanetType::WATER:
      MakeEarthAtmosphere(planet, 25);
      break;
    case PlanetType::DESERT:
      MakeEarthAtmosphere(planet, 50);
      seed(smap, SectorType::SEC_MOUNT,
           uniform_rand(total_sects / 50, total_sects / 25));
      grow(planet, smap, SectorType::SEC_MOUNT, 1, 1);
      grow(planet, smap, SectorType::SEC_MOUNT, 1, 2);
      seed(smap, SectorType::SEC_LAND,
           uniform_rand(total_sects / 50, total_sects / 25));
      grow(planet, smap, SectorType::SEC_LAND, 1, 1);
      grow(planet, smap, SectorType::SEC_LAND, 1, 3);
      grow(planet, smap, SectorType::SEC_DESERT, 1, 3);
      break;
  }
  Makesurface(planet,
              smap); /* determine surface geology based on environment */

  // Move sectormap to output parameter - caller will save after calculations
  out_smap = std::move(smap);
  return planet;
}

}  // namespace GB::creator
