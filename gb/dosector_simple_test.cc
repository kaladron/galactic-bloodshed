// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>
#include <cstring>

namespace {

// Helper function to create a test sector
Sector createTestSector(unsigned int x = 0, unsigned int y = 0,
                        unsigned int eff = 50, unsigned int fert = 50,
                        unsigned int mob = 0, unsigned int crystals = 0,
                        resource_t resource = 100, population_t popn = 1000,
                        population_t troops = 0, player_t owner = 1,
                        unsigned int type = SectorType::SEC_LAND,
                        unsigned int condition = SectorType::SEC_LAND) {
  return Sector(x, y, eff, fert, mob, crystals, resource, popn, troops, owner,
                owner, type, condition);
}

// Helper function to create a test race
Race createTestRace(player_t playernum = 1) {
  Race race{};  // Zero-initialize
  race.Playernum = playernum;
  race.metabolism = 1.0;
  race.birthrate = 0.1;
  race.number_sexes = 2;
  race.fertilize = 10;
  race.adventurism = 0.5;

  // Set sector compatibility
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    race.likes[i] = 0.8;
  }
  race.likes[SectorType::SEC_PLATED] = 1.0;
  race.likes[SectorType::SEC_WASTED] = 0.0;

  return race;
}

// Helper function to create a test planet
Planet createTestPlanet(unsigned char maxx = 10, unsigned char maxy = 10) {
  Planet planet(PlanetType::EARTH);
  planet.Maxx() = maxx;
  planet.Maxy() = maxy;
  planet.slaved_to() = 0;

  // Initialize conditions
  planet.conditions(TOXIC) = 0;

  // Initialize player info
  for (int i = 0; i < MAXPLAYERS; i++) {
    planet.info(i).tax = 0;
    planet.info(i).mob_set = 0;
    planet.info(i).resource = 0;
  }

  return planet;
}

// Helper function to create a test star
Star createTestStar() {
  star_struct star_data{};
  star_data.ships = 0;
  star_data.name = "TestStar";
  star_data.xpos = 0.0;
  star_data.ypos = 0.0;
  star_data.stability = 50;
  star_data.nova_stage = 0;
  star_data.temperature = 100;
  star_data.gravity = 1.0;
  star_data.star_id = 0;
  star_data.explored = 0;
  star_data.inhabited = 0;

  for (int i = 0; i < MAXPLAYERS; i++) {
    star_data.governor[i] = 0;
    star_data.AP[i] = 0;
  }

  star_data.pnames.push_back("TestPlanet");

  return Star(star_data);
}

// Test Sector data structure functionality
void test_sector_creation() {
  auto sector = createTestSector(5, 7, 80, 60, 25, 3, 200, 5000, 100, 2);

  // Test basic properties
  assert(sector.x == 5);
  assert(sector.y == 7);
  assert(sector.eff == 80);
  assert(sector.fert == 60);
  assert(sector.mobilization == 25);
  assert(sector.crystals == 3);
  assert(sector.resource == 200);
  assert(sector.popn == 5000);
  assert(sector.troops == 100);
  assert(sector.owner == 2);

  // Test sector types
  auto land_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  assert(land_sector.type == SectorType::SEC_LAND);
  assert(land_sector.condition == SectorType::SEC_LAND);

  auto plated_sector =
      createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_PLATED);
  assert(plated_sector.condition == SectorType::SEC_PLATED);

  auto wasted_sector =
      createTestSector(0, 0, 0, 0, 0, 0, 50, 0, 0, 0, SectorType::SEC_LAND,
                       SectorType::SEC_WASTED);
  assert(wasted_sector.condition == SectorType::SEC_WASTED);
}

// Test Race data structure functionality
void test_race_creation() {
  auto race = createTestRace(3);

  // Test basic properties
  assert(race.Playernum == 3);
  assert(race.metabolism == 1.0);
  assert(race.birthrate == 0.1);
  assert(race.number_sexes == 2);
  assert(race.fertilize == 10);
  assert(race.adventurism == 0.5);

  // Test sector compatibility
  assert(race.likes[SectorType::SEC_LAND] == 0.8);
  assert(race.likes[SectorType::SEC_PLATED] == 1.0);
  assert(race.likes[SectorType::SEC_WASTED] == 0.0);

  // Test race variations
  auto modified_race = createTestRace(1);
  modified_race.metabolism = 2.0;
  modified_race.birthrate = 0.2;
  modified_race.adventurism = 1.0;

  assert(modified_race.metabolism == 2.0);
  assert(modified_race.birthrate == 0.2);
  assert(modified_race.adventurism == 1.0);
}

// Test Planet data structure functionality
void test_planet_creation() {
  auto planet = createTestPlanet(15, 20);

  // Test basic properties
  assert(planet.Maxx() == 15);
  assert(planet.Maxy() == 20);
  assert(planet.slaved_to() == 0);
  assert(planet.conditions(TOXIC) == 0);

  // Test different planet types
  Planet earth_planet(PlanetType::EARTH);
  assert(earth_planet.type() == PlanetType::EARTH);

  Planet gas_planet(PlanetType::GASGIANT);
  assert(gas_planet.type() == PlanetType::GASGIANT);

  Planet asteroid(PlanetType::ASTEROID);
  assert(asteroid.type() == PlanetType::ASTEROID);

  // Test player info initialization
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(planet.info(i).tax == 0);
    assert(planet.info(i).mob_set == 0);
    assert(planet.info(i).resource == 0);
  }
}

// Test Star data structure functionality
void test_star_creation() {
  auto star = createTestStar();

  // Test basic properties
  assert(star.get_name() == "TestStar");
  assert(star.xpos() == 0.0);
  assert(star.ypos() == 0.0);
  assert(star.numplanets() == 1);
  assert(star.stability() == 50);
  assert(star.nova_stage() == 0);
  assert(star.temperature() == 100);
  assert(star.gravity() == 1.0);

  // Test modifications
  star.set_name("ModifiedStar");
  assert(star.get_name() == "ModifiedStar");

  star.xpos() = 100.5;
  star.ypos() = 200.7;
  assert(star.xpos() == 100.5);
  assert(star.ypos() == 200.7);

  star.stability() = 75;
  star.temperature() = 150;
  assert(star.stability() == 75);
  assert(star.temperature() == 150);

  // Test planet naming
  star.set_planet_name(0, "TestPlanet1");
  assert(star.get_planet_name(0) == "TestPlanet1");
}

// Test SectorMap functionality
void test_sectormap_functionality() {
  auto planet = createTestPlanet(5, 5);
  SectorMap smap(planet, true);

  // Test sector access and modification
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      auto& sector = smap.get(x, y);
      sector.x = x;
      sector.y = y;
      sector.owner = 1;
      sector.popn = 100 * (x + y);
      sector.condition = SectorType::SEC_LAND;

      // Verify the sector was set correctly
      assert(smap.get(x, y).x == x);
      assert(smap.get(x, y).y == y);
      assert(smap.get(x, y).owner == 1);
      assert(smap.get(x, y).popn == 100 * (x + y));
      assert(smap.get(x, y).condition == SectorType::SEC_LAND);
    }
  }

  // Test boundary conditions
  auto& corner_sector = smap.get(0, 0);
  corner_sector.eff = 100;
  corner_sector.fert = 50;
  corner_sector.resource = 1000;

  assert(smap.get(0, 0).eff == 100);
  assert(smap.get(0, 0).fert == 50);
  assert(smap.get(0, 0).resource == 1000);

  auto& opposite_corner = smap.get(4, 4);
  opposite_corner.crystals = 10;
  opposite_corner.mobilization = 75;

  assert(smap.get(4, 4).crystals == 10);
  assert(smap.get(4, 4).mobilization == 75);
}

// Test sector production calculations (mathematical functions)
void test_sector_calculations() {
  // Test maxsupport calculation
  auto race = createTestRace(1);
  auto sector = createTestSector(0, 0, 100, 80, 0, 0, 100, 1000, 0, 1);

  // maxsupport should be based on fertility and race compatibility
  population_t max_support = maxsupport(race, sector, 100.0, 0);
  assert(max_support > 0);  // Should be positive for good conditions

  // Test with different conditions
  population_t low_support =
      maxsupport(race, sector, 50.0, 50);  // Lower compat, toxic
  assert(low_support <= max_support);      // Should be lower or equal

  // Test with wasted sector
  sector.condition = SectorType::SEC_WASTED;
  population_t wasted_support = maxsupport(race, sector, 100.0, 0);
  assert(wasted_support < max_support);  // Wasted should support less
}

// Test boundary conditions and edge cases
void test_edge_cases() {
  // Test sectors with zero values
  auto empty_sector = createTestSector(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  assert(empty_sector.eff == 0);
  assert(empty_sector.fert == 0);
  assert(empty_sector.popn == 0);
  assert(empty_sector.owner == 0);

  // Test sectors with maximum values
  auto max_sector = createTestSector(255, 255, 100, 100, 100, 255, 65535,
                                     1000000, 1000000, MAXPLAYERS - 1);
  assert(max_sector.x == 255);
  assert(max_sector.y == 255);
  assert(max_sector.eff == 100);
  assert(max_sector.fert == 100);

  // Test race with extreme values
  auto extreme_race = createTestRace(MAXPLAYERS - 1);
  extreme_race.metabolism = 10.0;
  extreme_race.birthrate = 1.0;
  extreme_race.adventurism = 2.0;

  assert(extreme_race.Playernum == MAXPLAYERS - 1);
  assert(extreme_race.metabolism == 10.0);
  assert(extreme_race.birthrate == 1.0);
  assert(extreme_race.adventurism == 2.0);

  // Test planet with minimum/maximum sizes
  auto tiny_planet = createTestPlanet(1, 1);
  assert(tiny_planet.Maxx() == 1);
  assert(tiny_planet.Maxy() == 1);

  auto large_planet = createTestPlanet(100, 100);
  assert(large_planet.Maxx() == 100);
  assert(large_planet.Maxy() == 100);
}

// Test data consistency and relationships
void test_data_consistency() {
  // Test that sector types and conditions are consistent
  auto sector = createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                                 SectorType::SEC_LAND, SectorType::SEC_PLATED);

  // High efficiency should be consistent with plated condition
  assert(sector.eff == 100);
  assert(sector.condition == SectorType::SEC_PLATED);

  // Test race-sector compatibility relationships
  auto race = createTestRace(1);
  auto land_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  auto plated_sector =
      createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_PLATED);

  // Plated sectors should have higher compatibility
  assert(race.likes[SectorType::SEC_PLATED] > race.likes[SectorType::SEC_LAND]);

  // Test planet-star relationships
  auto star = createTestStar();
  auto planet = createTestPlanet(10, 10);

  // Planet should fit within reasonable star system constraints
  assert(planet.Maxx() >= 1 && planet.Maxx() <= 100);
  assert(planet.Maxy() >= 1 && planet.Maxy() <= 100);
  assert(star.numplanets() >= 0 && star.numplanets() <= MAXPLANETS);
}

}  // namespace

int main() noexcept {
  try {
    std::cout << "Running dosector simplified unit tests...\n";

    // Run all tests
    std::cout << "  Testing sector creation... ";
    test_sector_creation();
    std::cout << "PASS\n";

    std::cout << "  Testing race creation... ";
    test_race_creation();
    std::cout << "PASS\n";

    std::cout << "  Testing planet creation... ";
    test_planet_creation();
    std::cout << "PASS\n";

    std::cout << "  Testing star creation... ";
    test_star_creation();
    std::cout << "PASS\n";

    std::cout << "  Testing sectormap functionality... ";
    test_sectormap_functionality();
    std::cout << "PASS\n";

    std::cout << "  Testing sector calculations... ";
    test_sector_calculations();
    std::cout << "PASS\n";

    std::cout << "  Testing edge cases... ";
    test_edge_cases();
    std::cout << "PASS\n";

    std::cout << "  Testing data consistency... ";
    test_data_consistency();
    std::cout << "PASS\n";

    std::cout << "All dosector tests passed!\n";
    return 0;
  } catch (...) {
    std::cout << "Test failed with exception!\n";
    return 1;
  }
}
