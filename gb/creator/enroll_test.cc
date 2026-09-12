// SPDX-License-Identifier: Apache-2.0

/// \file enroll_test.cc
/// \brief Test race enrollment validation rules and star/planet candidate
/// selection.

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import gb.creator;
import strong_id;
import test;

namespace {

void test_enroll_first_race_god_requirement() {
  std::println(std::cout, "Test: First race enrolled must be God");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Mortal",
      .password = "secret",
      .is_god = false,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message,
                        "The first race enrolled must have God privileges.");

  std::println(std::cout, "  ✓ God race requirement check passed");
}

void test_enroll_max_players() {
  std::println(std::cout, "Test: Max player limit enforcement");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  RaceRepository races(store);

  for (int i = 1; i < MAXPLAYERS; ++i) {
    Race r{};
    r.Playernum = i;
    r.name = std::format("Race{}", i);
    races.save(r);
  }

  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Overflow",
      .password = "secret",
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message, "No more allowed.");

  std::println(std::cout, "  ✓ Max player limit enforcement passed");
}

void test_enroll_no_free_planet_type() {
  std::println(std::cout, "Test: No free home planet type rejection");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Sol";
  ss.pnames = {"MarsPlanet"};
  Star star(ss);
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{PlanetType::MARS, Coordinates{10, 10}};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Terrans",
      .password = "secret",
      .home_planet_type = PlanetType::EARTH,
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message, "Didn't find any free");

  std::println(std::cout, "  ✓ No free home planet type rejection passed");
}

void test_find_suitable_planet_deterministic_search() {
  std::println(std::cout,
               "Test: Deterministic find_suitable_planet exact search");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  universe_struct us{};
  us.id = 1;
  us.numstars = 6;
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);

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

  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  // Test 1: Given order [0, 1, 3, 2, 4, 5], should skip 0 and 1, and select
  // Star 3 (first valid candidate in order)
  std::vector<starnum_t> order1 = {0, 1, 3, 2, 4, 5};
  auto res1 = service.find_suitable_planet(PlanetType::EARTH, order1);
  test::expect_true(res1.has_value());
  if (!res1) return;
  test::expect_eq(res1->first, starnum_t{3});
  test::expect_eq(res1->second, planetnum_t{0});

  // Test 2: Given order [0, 1, 2, 3, 4, 5], should skip 0 and 1, and select
  // Star 2 (first valid candidate in order)
  std::vector<starnum_t> order2 = {0, 1, 2, 3, 4, 5};
  auto res2 = service.find_suitable_planet(PlanetType::EARTH, order2);
  test::expect_true(res2.has_value());
  if (!res2) return;
  test::expect_eq(res2->first, starnum_t{2});
  test::expect_eq(res2->second, planetnum_t{1});

  // Test 3: Gas Giant enrollment regression test (cold gas giant at -80C)
  auto res_gas = service.find_suitable_planet(PlanetType::GASGIANT, order2);
  test::expect_true(res_gas.has_value());
  if (!res_gas) return;
  test::expect_eq(res_gas->first, starnum_t{4});
  test::expect_eq(res_gas->second, planetnum_t{1});

  // Test 4: Cryogenic Iceball enrollment (cold iceball at -120C)
  auto res_ice = service.find_suitable_planet(PlanetType::ICEBALL, order2);
  test::expect_true(res_ice.has_value());
  if (!res_ice) return;
  test::expect_eq(res_ice->first, starnum_t{5});
  test::expect_eq(res_ice->second, planetnum_t{0});

  // Test 5: Hot Desert enrollment (warm desert world at 150C)
  auto res_desert = service.find_suitable_planet(PlanetType::DESERT, order2);
  test::expect_true(res_desert.has_value());
  if (!res_desert) return;
  test::expect_eq(res_desert->first, starnum_t{5});
  test::expect_eq(res_desert->second, planetnum_t{1});

  // Test 6: Looking for FOREST -> no matching planet -> returns std::nullopt
  auto res_none = service.find_suitable_planet(PlanetType::FOREST, order2);
  test::expect_false(res_none.has_value());

  std::println(std::cout, "  ✓ find_suitable_planet exact search passed");
}

void test_enroll_valid_race_success() {
  std::println(std::cout, "Test: enroll_valid_race success and bounds safety");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

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

  SectorRepository sector_repo(store);
  SectorMap smap(p1);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap.get(Coordinates{x, y}).set_condition(SectorType::SEC_GAS);
    }
  }
  sector_repo.save_map(smap);

  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Jovians",
      .password = "secret",
      .home_planet_type = PlanetType::GASGIANT,
      .preferred_sector = SectorType::SEC_GAS,
      .is_god = true,
      .mass = 1.0,
      .birthrate = 1.0,
      .fighters = 10,
      .iq = 100,
      .number_sexes = 1,
      .metabolism = 1.0,
      .likesbest = SectorType::SEC_GAS,
  };
  spec.sector_compatibilities[SectorType::SEC_GAS] = 1.0;

  auto result = service.enroll_player(spec);
  test::expect_true(result.success);
  test::expect_eq(result.player_num, player_t{1});

  const auto* enrolled_race = em.peek_race(player_t{1});
  test::expect_true(enrolled_race != nullptr);
  if (enrolled_race) {
    test::expect_eq(enrolled_race->name, std::string("Jovians"));
    test::expect_eq(enrolled_race->likesbest, SectorType::SEC_GAS);
    test::expect_eq(enrolled_race->likes[SectorType::SEC_GAS], 1.0);
    test::expect_eq(enrolled_race->likes[SectorType::SEC_WASTED], 0.0);
    test::expect_true(enrolled_race->God);
  }

  const auto* star = em.peek_star(0);
  test::expect_true(star != nullptr);
  if (star) {
    test::expect_true(star->is_explored_by(player_t{1}));
    test::expect_true(star->is_inhabited_by(player_t{1}));
  }

  const auto* planet = em.peek_planet(0, 1);
  test::expect_true(planet != nullptr);
  if (planet) {
    test::expect_gt(planet->popn(), 0);
  }
  const auto* gov_ship = em.peek_ship(enrolled_race->Gov_ship);
  test::expect_true(gov_ship != nullptr);
  if (gov_ship) {
    test::expect_eq(gov_ship->storbits(), starnum_t{0});
    test::expect_eq(gov_ship->pnumorbits(), planetnum_t{1});
  }

  std::println(std::cout, "  ✓ enroll_valid_race completed successfully");
}

}  // namespace

int main() {
  test_enroll_first_race_god_requirement();
  test_enroll_max_players();
  test_enroll_no_free_planet_type();
  test_find_suitable_planet_deterministic_search();
  test_enroll_valid_race_success();

  std::println(std::cout, "\n✅ All enroll tests passed!");
  return 0;
}
