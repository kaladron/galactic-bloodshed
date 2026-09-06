// SPDX-License-Identifier: Apache-2.0

/// \file enroll_test.cc
/// \brief Test race enrollment validation rules

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.server;
import test;

#include "gb/server/enroll.h"
#include "gb/server/racegen.h"

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

  Planet planet{PlanetType::MARS, Coordinates{10, 10}};
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

  Planet p2_0{PlanetType::MARS, Coordinates{10, 10}};
  p2_0.star_id() = 2;
  p2_0.planet_order() = 0;
  planet_repo.save(p2_0);

  Planet p2_1{PlanetType::EARTH, Coordinates{10, 10}};
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

  Planet p3_0{PlanetType::EARTH, Coordinates{10, 10}};
  p3_0.star_id() = 3;
  p3_0.planet_order() = 0;
  p3_0.conditions(RTEMP) = 15;
  planet_repo.save(p3_0);

  Planet p3_1{PlanetType::MARS, Coordinates{10, 10}};
  p3_1.star_id() = 3;
  p3_1.planet_order() = 1;
  planet_repo.save(p3_1);

  // Star 4: 2 planets, candidate Gas Giant at pnum 1 (cold: RTEMP = -80)
  star_struct ss4{};
  ss4.star_id = 4;
  ss4.inhabited = 0;
  ss4.pnames = {"P1", "P2"};
  Star star4(ss4);
  star_repo.save(star4);

  Planet p4_0{PlanetType::MARS, Coordinates{10, 10}};
  p4_0.star_id() = 4;
  p4_0.planet_order() = 0;
  planet_repo.save(p4_0);

  Planet p4_1{PlanetType::GASGIANT, Coordinates{10, 10}};
  p4_1.star_id() = 4;
  p4_1.planet_order() = 1;
  p4_1.conditions(RTEMP) = -80;
  planet_repo.save(p4_1);

  // Star 5: 2 planets, cryogenic Iceball at pnum 0 (RTEMP = -120), hot Desert
  // at pnum 1 (RTEMP = 150)
  star_struct ss5{};
  ss5.star_id = 5;
  ss5.inhabited = 0;
  ss5.pnames = {"P1", "P2"};
  Star star5(ss5);
  star_repo.save(star5);

  Planet p5_0{PlanetType::ICEBALL, Coordinates{10, 10}};
  p5_0.star_id() = 5;
  p5_0.planet_order() = 0;
  p5_0.conditions(RTEMP) = -120;
  planet_repo.save(p5_0);

  Planet p5_1{PlanetType::DESERT, Coordinates{10, 10}};
  p5_1.star_id() = 5;
  p5_1.planet_order() = 1;
  p5_1.conditions(RTEMP) = 150;
  planet_repo.save(p5_1);

  // Test 1: Given order [0, 1, 3, 2, 4, 5], should skip 0 and 1, and select
  // Star 3 (first valid candidate in order)
  std::vector<int> order1 = {0, 1, 3, 2, 4, 5};
  auto res1 = find_suitable_enrol_planet(em, 6, 1, PlanetType::EARTH, order1);
  test::expect_true(res1.has_value());
  if (!res1) return;
  test::expect_eq(res1->first, 3);
  test::expect_eq(res1->second, 0);

  // Test 2: Given order [0, 1, 2, 3, 4, 5], should skip 0 and 1, and select
  // Star 2 (first valid candidate in order)
  std::vector<int> order2 = {0, 1, 2, 3, 4, 5};
  auto res2 = find_suitable_enrol_planet(em, 6, 1, PlanetType::EARTH, order2);
  test::expect_true(res2.has_value());
  if (!res2) return;
  test::expect_eq(res2->first, 2);
  test::expect_eq(res2->second, 1);

  // Test 3: Gas Giant enrollment regression test (cold gas giant at -80C)
  auto res_gas =
      find_suitable_enrol_planet(em, 6, 1, PlanetType::GASGIANT, order2);
  test::expect_true(res_gas.has_value());
  if (!res_gas) return;
  test::expect_eq(res_gas->first, 4);
  test::expect_eq(res_gas->second, 1);

  // Test 4: Cryogenic Iceball enrollment (cold iceball at -120C)
  auto res_ice =
      find_suitable_enrol_planet(em, 6, 1, PlanetType::ICEBALL, order2);
  test::expect_true(res_ice.has_value());
  if (!res_ice) return;
  test::expect_eq(res_ice->first, 5);
  test::expect_eq(res_ice->second, 0);

  // Test 5: Hot Desert enrollment (warm desert world at 150C)
  auto res_desert =
      find_suitable_enrol_planet(em, 6, 1, PlanetType::DESERT, order2);
  test::expect_true(res_desert.has_value());
  if (!res_desert) return;
  test::expect_eq(res_desert->first, 5);
  test::expect_eq(res_desert->second, 1);

  // Test 6: Looking for FOREST -> no matching planet -> returns std::nullopt
  auto res_none =
      find_suitable_enrol_planet(em, 6, 1, PlanetType::FOREST, order2);
  test::expect_false(res_none.has_value());

  std::println(std::cout, "  ✓ find_suitable_enrol_planet exact search passed");
}

void test_racegen_db_path_config() {
  std::println(std::cout, "Test: racegen database path configuration");

  // Default path should end with gb.db
  test::expect_contains(get_racegen_db_path(), "gb.db");

  // Setting custom path
  set_racegen_db_path("/custom/path/to/game.sqlite");
  test::expect_eq(get_racegen_db_path(),
                  std::string("/custom/path/to/game.sqlite"));

  // Reset back to default
  set_racegen_db_path(PKGSTATEDIR "gb.db");
  test::expect_eq(get_racegen_db_path(), std::string(PKGSTATEDIR "gb.db"));

  std::println(std::cout, "  ✓ racegen database path configuration passed");
}

void test_enroll_valid_race_success() {
  std::println(std::cout, "Test: enroll_valid_race success and bounds safety");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  // Setup: Create universe with 1 star
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  // Star 0 has 2 planets: Planet 0 is MARS, Planet 1 is GASGIANT
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.inhabited = 0;
  ss0.pnames = {"Ares", "Jupiter"};
  Star star0(ss0);
  StarRepository star_repo(store);
  star_repo.save(star0);

  PlanetRepository planet_repo(store);
  Planet p0{PlanetType::MARS, Coordinates{5, 5}};
  p0.star_id() = 0;
  p0.planet_order() = 0;
  planet_repo.save(p0);

  Planet p1{PlanetType::GASGIANT, Coordinates{5, 5}};
  p1.star_id() = 0;
  p1.planet_order() = 1;
  p1.conditions(RTEMP) = -80;
  planet_repo.save(p1);

  // Populate SectorMap with SEC_GAS sectors so capital sector preference
  // matches
  SectorRepository sector_repo(store);
  SectorMap smap(p1);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap.get(Coordinates{x, y}).set_condition(SectorType::SEC_GAS);
    }
  }
  sector_repo.save_map(smap);

  // Setup: Jovian God race
  race_info = x{};
  race_info.priv_type = P_GOD;
  std::snprintf(race_info.name, sizeof(race_info.name), "Jovians");
  std::snprintf(race_info.password, sizeof(race_info.password), "secret");
  race_info.home_planet_type = H_JOVIAN;
  race_info.attr[SEXES] = 1.0;
  race_info.attr[A_IQ] = 100.0;
  race_info.attr[BIRTH] = 1.0;
  race_info.attr[MASS] = 1.0;
  race_info.attr[METAB] = 1.0;
  race_info.compat[S_GAS] = 100.0;

  // TEST: Execute enroll_valid_race
  int result = enroll_valid_race(db);

  // Verify: Successfully enrolled
  test::expect_eq(result, 0);
  test::expect_eq(race_info.status, EnrollmentStatus::ENROLLED);

  // Verify: Race created with correct sector preferences and bounds
  EntityManager em(db);
  const auto* enrolled_race = em.peek_race(player_t{1});
  test::expect_true(enrolled_race != nullptr);
  if (enrolled_race) {
    test::expect_eq(enrolled_race->name, std::string("Jovians"));
    test::expect_eq(enrolled_race->likesbest, SectorType::SEC_GAS);
    test::expect_eq(enrolled_race->likes[SectorType::SEC_GAS], 1.0);
    test::expect_eq(enrolled_race->likes[SectorType::SEC_WASTED], 0.0);
    test::expect_true(enrolled_race->God);
  }

  // Verify: Star is marked explored and inhabited
  const auto* star = em.peek_star(0);
  test::expect_true(star != nullptr);
  if (star) {
    test::expect_true(star->is_explored_by(player_t{1}));
    test::expect_true(star->is_inhabited_by(player_t{1}));
  }

  // Verify: Planet population and governor ship created
  const auto* planet = em.peek_planet(0, 1);
  test::expect_true(planet != nullptr);
  if (planet) {
    test::expect_gt(planet->popn(), 0);
    test::expect_eq(planet->ships(), enrolled_race->Gov_ship);
  }

  std::println(std::cout, "  ✓ enroll_valid_race completed successfully");
}

int main() {
  test_enroll_first_race_god_requirement();
  test_enroll_max_players();
  test_enroll_no_free_planet_type();
  test_find_suitable_enrol_planet();
  test_racegen_db_path_config();
  test_enroll_valid_race_success();

  std::println(std::cout, "\n✅ All enroll tests passed!");
  return 0;
}
