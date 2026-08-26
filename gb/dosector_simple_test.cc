// SPDX-License-Identifier: Apache-2.0

/// \file dosector_simple_test.cc
/// \brief Unit tests for Sector data structures, calculations, efficiency
/// plating, population spread, and troop maintenance.

import dallib;
import gblib;
import test;
import std;

#include <cstring>

namespace {

// Helper function to create a test sector
Sector createTestSector(unsigned int x = 0, unsigned int y = 0,
                        unsigned int eff = 50, unsigned int fert = 50,
                        unsigned int mob = 0, unsigned int crystals = 0,
                        resource_t resource = 100, population_t popn = 1000,
                        population_t troops = 0, player_t owner = 1,
                        SectorType type = SectorType::SEC_LAND,
                        SectorType condition = SectorType::SEC_LAND) {
  return Sector(sector_struct{
      .coords = {static_cast<int>(x), static_cast<int>(y)},
      .eff = eff,
      .fert = fert,
      .mobilization = mob,
      .crystals = crystals,
      .resource = resource,
      .popn = popn,
      .troops = troops,
      .owner = owner,
      .race = owner,
      .type = type,
      .condition = condition,
  });
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
  Planet planet(PlanetType::EARTH, Coordinates{maxx, maxy});
  planet.Maxx() = maxx;
  planet.Maxy() = maxy;
  planet.slaved_to() = 0;

  // Initialize conditions
  planet.conditions(TOXIC) = 0;

  // Initialize player info
  for (int i = 1; i <= MAXPLAYERS; i++) {
    planet.info(player_t{i}).tax = 0;
    planet.info(player_t{i}).mob_set = 0;
    planet.info(player_t{i}).resource = 0;
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
  test::expect_eq(sector.get_x(), 5);
  test::expect_eq(sector.get_y(), 7);
  test::expect_eq(sector.get_eff(), 80);
  test::expect_eq(sector.get_fert(), 60);
  test::expect_eq(sector.get_mobilization(), 25);
  test::expect_eq(sector.get_crystals(), 3);
  test::expect_eq(sector.get_resource(), 200);
  test::expect_eq(sector.get_popn(), 5000);
  test::expect_eq(sector.get_troops(), 100);
  test::expect_eq(sector.get_owner(), 2);

  // Test sector types
  auto land_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  test::expect_eq(land_sector.get_type(), SectorType::SEC_LAND);
  test::expect_eq(land_sector.get_condition(), SectorType::SEC_LAND);
  test::expect_false(land_sector.is_plated());
  test::expect_false(land_sector.is_wasted());

  auto plated_sector =
      createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_PLATED);
  test::expect_eq(plated_sector.get_condition(), SectorType::SEC_PLATED);
  test::expect_true(plated_sector.is_plated());
  test::expect_false(plated_sector.is_wasted());

  auto wasted_sector =
      createTestSector(0, 0, 0, 0, 0, 0, 50, 0, 0, 0, SectorType::SEC_LAND,
                       SectorType::SEC_WASTED);
  test::expect_eq(wasted_sector.get_condition(), SectorType::SEC_WASTED);
  test::expect_true(wasted_sector.is_wasted());
  test::expect_false(wasted_sector.is_plated());

  // Test is_owned() method
  auto owned_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  test::expect_true(owned_sector.is_owned());

  auto unowned_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 0, 0, 0, SectorType::SEC_LAND,
                       SectorType::SEC_LAND);
  test::expect_false(unowned_sector.is_owned());

  // Test is_empty() method
  auto empty_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 0, 0, 1, SectorType::SEC_LAND,
                       SectorType::SEC_LAND);
  test::expect_true(empty_sector.is_empty());

  auto populated_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  test::expect_false(populated_sector.is_empty());

  auto troops_only_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 0, 50, 1, SectorType::SEC_LAND,
                       SectorType::SEC_LAND);
  test::expect_false(troops_only_sector.is_empty());

  // Test plate() method
  auto unplated_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  test::expect_false(unplated_sector.is_plated());
  test::expect_eq(unplated_sector.get_eff(), 50);
  unplated_sector.plate();
  test::expect_true(unplated_sector.is_plated());
  test::expect_eq(unplated_sector.get_eff(), 100);

  // Gas sectors don't get SEC_PLATED condition
  auto gas_sector = createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                                     SectorType::SEC_GAS, SectorType::SEC_GAS);
  gas_sector.plate();
  test::expect_eq(gas_sector.get_eff(), 100);
  test::expect_eq(gas_sector.get_condition(), SectorType::SEC_GAS);

  // Test clear_owner_if_empty() method
  auto sector_with_popn =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  sector_with_popn.clear_owner_if_empty();
  test::expect_eq(sector_with_popn.get_owner(), 1);  // Still owned

  auto sector_empty_owned =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 0, 0, 1, SectorType::SEC_LAND,
                       SectorType::SEC_LAND);
  test::expect_eq(sector_empty_owned.get_owner(), 1);
  sector_empty_owned.clear_owner_if_empty();
  test::expect_eq(sector_empty_owned.get_owner(), 0);  // Now unowned
}

// Test Race data structure functionality
void test_race_creation() {
  auto race = createTestRace(3);

  // Test basic properties
  test::expect_eq(race.Playernum, 3);
  test::expect_eq(race.metabolism, 1.0);
  test::expect_eq(race.birthrate, 0.1);
  test::expect_eq(race.number_sexes, 2);
  test::expect_eq(race.fertilize, 10);
  test::expect_eq(race.adventurism, 0.5);

  // Test sector compatibility
  test::expect_eq(race.likes[SectorType::SEC_LAND], 0.8);
  test::expect_eq(race.likes[SectorType::SEC_PLATED], 1.0);
  test::expect_eq(race.likes[SectorType::SEC_WASTED], 0.0);

  // Test race variations
  auto modified_race = createTestRace(1);
  modified_race.metabolism = 2.0;
  modified_race.birthrate = 0.2;
  modified_race.adventurism = 1.0;

  test::expect_eq(modified_race.metabolism, 2.0);
  test::expect_eq(modified_race.birthrate, 0.2);
  test::expect_eq(modified_race.adventurism, 1.0);
}

// Test Planet data structure functionality
void test_planet_creation() {
  auto planet = createTestPlanet(15, 20);

  // Test basic properties
  test::expect_eq(planet.Maxx(), 15);
  test::expect_eq(planet.Maxy(), 20);
  test::expect_eq(planet.slaved_to(), 0);
  test::expect_eq(planet.conditions(TOXIC), 0);

  // Test different planet types
  Planet earth_planet(PlanetType::EARTH, Coordinates{10, 10});
  test::expect_eq(earth_planet.type(), PlanetType::EARTH);

  Planet gas_planet(PlanetType::GASGIANT, Coordinates{10, 10});
  test::expect_eq(gas_planet.type(), PlanetType::GASGIANT);

  Planet asteroid(PlanetType::ASTEROID, Coordinates{10, 10});
  test::expect_eq(asteroid.type(), PlanetType::ASTEROID);

  // Test player info initialization
  for (int i = 1; i <= MAXPLAYERS; i++) {
    test::expect_eq(planet.info(player_t{i}).tax, 0);
    test::expect_eq(planet.info(player_t{i}).mob_set, 0);
    test::expect_eq(planet.info(player_t{i}).resource, 0);
  }
}

// Test Star data structure functionality
void test_star_creation() {
  auto star = createTestStar();

  // Test basic properties
  test::expect_eq(star.get_name(), "TestStar");
  test::expect_eq(star.xpos(), 0.0);
  test::expect_eq(star.ypos(), 0.0);
  test::expect_eq(star.numplanets(), 1);
  test::expect_eq(star.stability(), 50);
  test::expect_eq(star.nova_stage(), 0);
  test::expect_eq(star.temperature(), 100);
  test::expect_eq(star.gravity(), 1.0);

  // Test modifications
  star.set_name("ModifiedStar");
  test::expect_eq(star.get_name(), "ModifiedStar");

  star.xpos() = 100.5;
  star.ypos() = 200.7;
  test::expect_eq(star.xpos(), 100.5);
  test::expect_eq(star.ypos(), 200.7);

  star.stability() = 75;
  star.temperature() = 150;
  test::expect_eq(star.stability(), 75);
  test::expect_eq(star.temperature(), 150);

  // Test planet naming
  star.set_planet_name(0, "TestPlanet1");
  test::expect_eq(star.get_planet_name(0), "TestPlanet1");
}

// Test SectorMap functionality
void test_sectormap_functionality() {
  auto planet = createTestPlanet(5, 5);
  SectorMap smap(planet);

  // Test sector access and modification
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      auto& sector = smap.get(Coordinates{x, y});
      sector.set_x(x);
      sector.set_y(y);
      sector.set_owner(1);
      sector.set_popn_exact(100 * (x + y));
      sector.set_condition(SectorType::SEC_LAND);

      // Verify the sector was set correctly
      test::expect_eq(smap.get(Coordinates{x, y}).get_x(), x);
      test::expect_eq(smap.get(Coordinates{x, y}).get_y(), y);
      test::expect_eq(smap.get(Coordinates{x, y}).get_owner(), 1);
      test::expect_eq(smap.get(Coordinates{x, y}).get_popn(), 100 * (x + y));
      test::expect_eq(smap.get(Coordinates{x, y}).get_condition(),
                      SectorType::SEC_LAND);
    }
  }

  // Test boundary conditions
  auto& corner_sector = smap.get(Coordinates{0, 0});
  corner_sector.set_efficiency_bounded(100);
  corner_sector.set_fert(50);
  corner_sector.set_resource(1000);

  test::expect_eq(smap.get(Coordinates{0, 0}).get_eff(), 100);
  test::expect_eq(smap.get(Coordinates{0, 0}).get_fert(), 50);
  test::expect_eq(smap.get(Coordinates{0, 0}).get_resource(), 1000);

  auto& opposite_corner = smap.get(Coordinates{4, 4});
  opposite_corner.set_crystals(10);
  opposite_corner.set_mobilization(75);

  test::expect_eq(smap.get(Coordinates{4, 4}).get_crystals(), 10);
  test::expect_eq(smap.get(Coordinates{4, 4}).get_mobilization(), 75);
}

// Test sector production calculations (mathematical functions)
void test_sector_calculations() {
  // Test maxsupport calculation
  auto race = createTestRace(1);
  auto sector = createTestSector(0, 0, 100, 80, 0, 0, 100, 1000, 0, 1);

  // maxsupport should be based on fertility and race compatibility
  population_t max_support = maxsupport(race, sector, 100.0, 0);
  test::expect_gt(max_support, 0);  // Should be positive for good conditions

  // Test with different conditions
  population_t low_support =
      maxsupport(race, sector, 50.0, 50);     // Lower compat, toxic
  test::expect_le(low_support, max_support);  // Should be lower or equal

  // Test with wasted sector
  sector.set_condition(SectorType::SEC_WASTED);
  population_t wasted_support = maxsupport(race, sector, 100.0, 0);
  test::expect_lt(wasted_support, max_support);  // Wasted should support less
}

// Test boundary conditions and edge cases
void test_edge_cases() {
  // Test sectors with zero values
  auto empty_sector = createTestSector(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  test::expect_eq(empty_sector.get_eff(), 0);
  test::expect_eq(empty_sector.get_fert(), 0);
  test::expect_eq(empty_sector.get_popn(), 0);
  test::expect_eq(empty_sector.get_owner(), 0);

  // Test sectors with maximum values
  auto max_sector = createTestSector(255, 255, 100, 100, 100, 255, 65535,
                                     1000000, 1000000, MAXPLAYERS - 1);
  test::expect_eq(max_sector.get_x(), 255);
  test::expect_eq(max_sector.get_y(), 255);
  test::expect_eq(max_sector.get_eff(), 100);
  test::expect_eq(max_sector.get_fert(), 100);

  // Test race with extreme values
  auto extreme_race = createTestRace(MAXPLAYERS - 1);
  extreme_race.metabolism = 10.0;
  extreme_race.birthrate = 1.0;
  extreme_race.adventurism = 2.0;

  test::expect_eq(extreme_race.Playernum, MAXPLAYERS - 1);
  test::expect_eq(extreme_race.metabolism, 10.0);
  test::expect_eq(extreme_race.birthrate, 1.0);
  test::expect_eq(extreme_race.adventurism, 2.0);

  // Test planet with minimum/maximum sizes
  auto tiny_planet = createTestPlanet(1, 1);
  test::expect_eq(tiny_planet.Maxx(), 1);
  test::expect_eq(tiny_planet.Maxy(), 1);

  auto large_planet = createTestPlanet(100, 100);
  test::expect_eq(large_planet.Maxx(), 100);
  test::expect_eq(large_planet.Maxy(), 100);
}

// Test data consistency and relationships
void test_data_consistency() {
  // Test that sector types and conditions are consistent
  auto sector = createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                                 SectorType::SEC_LAND, SectorType::SEC_PLATED);

  // High efficiency should be consistent with plated condition
  test::expect_eq(sector.get_eff(), 100);
  test::expect_true(sector.is_plated());

  // Test race-sector compatibility relationships
  auto race = createTestRace(1);
  auto land_sector =
      createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_LAND);
  auto plated_sector =
      createTestSector(0, 0, 100, 50, 0, 0, 100, 1000, 0, 1,
                       SectorType::SEC_LAND, SectorType::SEC_PLATED);

  // Plated sectors should have higher compatibility
  test::expect_gt(race.likes[SectorType::SEC_PLATED],
                  race.likes[SectorType::SEC_LAND]);

  // Plated sectors should support more population
  population_t land_support = maxsupport(race, land_sector, 100.0, 0);
  population_t plated_support = maxsupport(race, plated_sector, 100.0, 0);
  test::expect_gt(plated_support, land_support);

  // Verify sector properties are as expected
  test::expect_false(land_sector.is_plated());
  test::expect_true(plated_sector.is_plated());
  test::expect_eq(plated_sector.get_eff(), 100);

  // Test planet-star relationships
  auto star = createTestStar();
  auto planet = createTestPlanet(10, 10);

  // Planet should fit within reasonable star system constraints
  test::expect_ge(planet.Maxx(), 1);
  test::expect_le(planet.Maxx(), 100);
  test::expect_ge(planet.Maxy(), 1);
  test::expect_le(planet.Maxy(), 100);
  test::expect_ge(star.numplanets(), 0);
  test::expect_le(star.numplanets(), MAXPLANETS);
}

void test_produce_and_troop_maintenance() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.governor[0].maintain = 0;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  Planet planet = createTestPlanet(5, 5);

  Sector s = createTestSector(0, 0, 50, 50, 0, 0, 100, 1000, 50, player_t{1});
  TurnStats stats{};
  stats.Compat[player_t{1}] = 1.0;

  produce(em, star, planet, s, stats);

  const auto* updated_race = em.peek_race(player_t{1});
  test::expect_ne(updated_race, nullptr);
  test::expect_eq(updated_race->governor[0].maintain, UPDATE_TROOP_COST * 50);
}

void test_spread_population() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.adventurism = 1.0;
  RaceRepository races(store);
  races.save(race);

  Planet planet = createTestPlanet(5, 5);
  SectorMap smap(planet);

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      smap.get(Coordinates{x, y}).set_owner(0);
      smap.get(Coordinates{x, y}).clear_popn();
      smap.get(Coordinates{x, y}).set_condition(SectorType::SEC_LAND);
    }
  }

  auto& center = smap.get(Coordinates{2, 2});
  center.set_x(2);
  center.set_y(2);
  center.set_owner(1);
  center.set_popn_exact(10000);
  center.set_fert(0);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 1.0;

  spread(em, planet, center, smap, stats);

  bool spread_occurred = false;
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      if ((x != 2 || y != 2) && smap.get(Coordinates{x, y}).get_owner() == 1) {
        spread_occurred = true;
        break;
      }
    }
  }
  test::expect_true(spread_occurred);
}

}  // namespace

int main() {
  std::println(std::cout, "Running dosector simplified unit tests...\n");

  std::println(std::cout, "  Testing sector creation... ");
  test_sector_creation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing race creation... ");
  test_race_creation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing planet creation... ");
  test_planet_creation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing star creation... ");
  test_star_creation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing sectormap functionality... ");
  test_sectormap_functionality();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing sector calculations... ");
  test_sector_calculations();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing edge cases... ");
  test_edge_cases();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing data consistency... ");
  test_data_consistency();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing produce and troop maintenance... ");
  test_produce_and_troop_maintenance();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing population spread... ");
  test_spread_population();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All dosector tests passed!");
  return 0;
}
