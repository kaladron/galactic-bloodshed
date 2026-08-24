// SPDX-License-Identifier: Apache-2.0

/// \file enroll_test.cc
/// \brief Test race enrollment validation rules

import std;
import dallib;
import gblib;
import test;

#include "gb/enroll.h"
#include "gb/racegen.h"

int enroll_valid_race(Database& db);

// Global variable definitions required by GB_racegen.cc
struct x race_info{};
const char* planet_print_name[N_HOME_PLANET_TYPES] = {
    "Earth", "Forest", "Desert", "Water", "Airless", "Iceball", "Jovian"};
const double planet_compat_cov[N_HOME_PLANET_TYPES][N_SECTOR_TYPES] = {
    {1.00, 1.00, 2.00, 99.00, 1.01, 1.50, 3.00, 1.01},
    {1.01, 1.50, 2.00, 99.00, 1.01, 1.00, 3.00, 1.01},
    {3.00, 1.01, 1.01, 99.00, 1.50, 3.00, 1.00, 1.01},
    {1.00, 1.50, 3.00, 99.00, 1.01, 1.01, 3.00, 1.01},
    {1.01, 1.00, 1.00, 99.00, 1.01, 1.01, 1.00, 1.01},
    {3.00, 1.01, 1.00, 99.00, 1.00, 1.50, 2.00, 1.01},
    {99.00, 99.00, 99.00, 1.00, 99.00, 99.00, 99.00, 99.00}};

void test_enroll_first_race_god_requirement() {
  std::println(std::cout, "Test: First race enrolled must be God");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);

  // Setup: Set race_info to non-God race
  race_info = x{};
  race_info.priv_type = P_NORMAL;

  // TEST: Attempt to enroll non-God race as player 1
  int result = enroll_valid_race(db);

  // Verify: Enrollment fails with God privilege error
  test::expect_eq(result, 1);
  test::expect_contains(std::string(race_info.rejection),
                        "The first race enrolled must have God privileges.");

  std::println(std::cout, "  ✓ God race requirement check passed");
}

void test_enroll_max_players() {
  std::println(std::cout, "Test: Max player limit enforcement");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);

  // Setup: Save MAXPLAYERS-1 dummy races to fill the database
  JsonStore store(db);
  RaceRepository races(store);

  for (int i = 1; i < MAXPLAYERS; ++i) {
    Race r{};
    r.Playernum = i;
    r.name = std::format("Race{}", i);
    races.save(r);
  }

  // Setup: Prepare new race for enrollment
  race_info = x{};
  race_info.priv_type = P_GOD;

  // TEST: Attempt to enroll when MAXPLAYERS is reached
  int result = enroll_valid_race(db);

  // Verify: Enrollment rejected due to max players limit
  test::expect_eq(result, 1);
  test::expect_eq(race_info.status, EnrollmentStatus::UNENROLLABLE);
  test::expect_contains(race_info.rejection, "No more allowed.");

  std::println(std::cout, "  ✓ Max player limit enforcement passed");
}

void test_enroll_no_free_planet_type() {
  std::println(std::cout, "Test: No free home planet type rejection");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  // Setup: Create universe with 1 star, 1 planet of type MARS
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Sol";
  ss.pnames.emplace_back("MarsPlanet");
  Star star(ss);
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{PlanetType::MARS};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Prepare race_info seeking EARTH planet (type 0)
  race_info = x{};
  race_info.priv_type = P_GOD;
  race_info.home_planet_type = H_EARTH;

  int result = enroll_valid_race(db);

  test::expect_eq(result, 1);
  test::expect_eq(race_info.status, EnrollmentStatus::UNENROLLABLE);
  test::expect_contains(race_info.rejection, "Didn't find any free Earth");

  std::println(std::cout, "  ✓ No free home planet type rejection passed");
}

void test_find_suitable_enrol_planet() {
  std::println(std::cout,
               "Test: Deterministic find_suitable_enrol_planet exact search");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  EntityManager em(db);

  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);

  // Setup multiple stars:
  // Star 0: Inhabited -> skip
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.inhabited = 1;
  ss0.pnames = {"P1", "P2"};
  Star star0(ss0);
  star_repo.save(star0);

  // Star 1: Only 1 planet -> skip
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.inhabited = 0;
  ss1.pnames = {"P1"};
  Star star1(ss1);
  star_repo.save(star1);

  // Star 2: 2 planets, candidate Earth planet at pnum 1 (valid)
  star_struct ss2{};
  ss2.star_id = 2;
  ss2.inhabited = 0;
  ss2.pnames = {"P1", "P2"};
  Star star2(ss2);
  star_repo.save(star2);

  Planet p2_0{PlanetType::MARS};
  p2_0.star_id() = 2;
  p2_0.planet_order() = 0;
  planet_repo.save(p2_0);

  Planet p2_1{PlanetType::EARTH};
  p2_1.star_id() = 2;
  p2_1.planet_order() = 1;
  p2_1.conditions(RTEMP) = 20;
  planet_repo.save(p2_1);

  // Star 3: 2 planets, candidate Earth planet at pnum 0 (valid)
  star_struct ss3{};
  ss3.star_id = 3;
  ss3.inhabited = 0;
  ss3.pnames = {"P1", "P2"};
  Star star3(ss3);
  star_repo.save(star3);

  Planet p3_0{PlanetType::EARTH};
  p3_0.star_id() = 3;
  p3_0.planet_order() = 0;
  p3_0.conditions(RTEMP) = 15;
  planet_repo.save(p3_0);

  Planet p3_1{PlanetType::MARS};
  p3_1.star_id() = 3;
  p3_1.planet_order() = 1;
  planet_repo.save(p3_1);

  // Test 1: Given order [0, 1, 3, 2], should skip 0 and 1, and select Star 3
  // (first valid candidate in order)
  std::vector<int> order1 = {0, 1, 3, 2};
  auto res1 = find_suitable_enrol_planet(em, 4, 1, PlanetType::EARTH, order1);
  test::expect_true(res1.has_value());
  test::expect_eq(res1->first, 3);
  test::expect_eq(res1->second, 0);

  // Test 2: Given order [0, 1, 2, 3], should skip 0 and 1, and select Star 2
  // (first valid candidate in order)
  std::vector<int> order2 = {0, 1, 2, 3};
  auto res2 = find_suitable_enrol_planet(em, 4, 1, PlanetType::EARTH, order2);
  test::expect_true(res2.has_value());
  test::expect_eq(res2->first, 2);
  test::expect_eq(res2->second, 1);

  // Test 3: Looking for DESERT -> no matching planet -> returns std::nullopt
  auto res3 = find_suitable_enrol_planet(em, 4, 1, PlanetType::DESERT, order2);
  test::expect_false(res3.has_value());

  std::println(std::cout, "  ✓ find_suitable_enrol_planet exact search passed");
}

int main() {
  test_enroll_first_race_god_requirement();
  test_enroll_max_players();
  test_enroll_no_free_planet_type();
  test_find_suitable_enrol_planet();

  std::println(std::cout, "\n✅ All enroll tests passed!");
  return 0;
}
