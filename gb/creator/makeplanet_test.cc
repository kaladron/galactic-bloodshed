// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

#include "gb/creator/makeplanet.h"
#include "gb/creator/makestar.h"
#include "gb/creator/makeuniv.h"

/// \file makeplanet_test.cc
/// \brief Test temperature calculation and planet generation

// Define global stubs required by makestar.cc
int autoname_plan = 0;
int autoname_star = 0;
int minplanets = 1;
int maxplanets = 10;
int printplaninfo = 0;
int printstarinfo = 0;
void place_star(star_struct&) {}

void test_temperature_calculation() {
  std::println(std::cout, "Test: Temperature calculation");

  // TEST: Calculate temperatures at increasing orbital distances from star
  int t1 = Temperature(100.0, 5000);
  int t2 = Temperature(500.0, 5000);
  int t3 = Temperature(1500.0, 5000);

  // Verify: Farther planets must be colder than closer planets
  assert(t1 > t2);
  assert(t2 > t3);

  std::println(
      std::cout,
      "  ✓ Temperature calculation formula works (t1={}, t2={}, t3={})", t1, t2,
      t3);
}

void test_makeplanet_types() {
  std::println(std::cout, "Test: makeplanet for all PlanetTypes");

  // Setup: Target star ID and list of planet types to test
  starnum_t star_id{1};

  std::vector<PlanetType> types = {PlanetType::EARTH,    PlanetType::MARS,
                                   PlanetType::GASGIANT, PlanetType::DESERT,
                                   PlanetType::WATER,    PlanetType::ICEBALL,
                                   PlanetType::ASTEROID};

  for (std::size_t i = 0; i < types.size(); ++i) {
    PlanetType ptype = types[i];
    planetnum_t pnum{static_cast<unsigned int>(i)};
    std::optional<SectorMap> smap;

    // TEST: Generate planet with makeplanet()
    Planet planet = makeplanet(500.0, 6000, ptype, star_id, pnum, smap);

    // Verify: Planet type, location, and dimensions
    assert(planet.type() == ptype);
    assert(planet.star_id() == star_id);
    assert(planet.planet_order() == pnum);
    assert(planet.Maxx() > 0);
    assert(planet.Maxy() > 0);

    // Verify: Solid planets have generated sector maps
    if (ptype != PlanetType::GASGIANT) {
      assert(smap.has_value());
    }

    std::println(std::cout,
                 "  ✓ Planet type {} generated with dimensions {}x{}",
                 static_cast<int>(ptype), planet.Maxx(), planet.Maxy());
  }
}

int main() {
  test_temperature_calculation();
  test_makeplanet_types();

  std::println(std::cout, "\n✅ All makeplanet tests passed!");
  return 0;
}
