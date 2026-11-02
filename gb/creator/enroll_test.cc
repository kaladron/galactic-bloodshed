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

void test_seed_deity_and_mortal_enrollment() {
  std::println(std::cout, "Test: seed_deity and mortal enrollment");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  Universe us{};
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  EntityManager em(db);

  TestStarBuilder(em, db, "Sol", 1).build();
  TestPlanetBuilder(em, db, 1, PlanetType::EARTH, Coordinates{5, 5}, 1)
      .named("Terra")
      .with_temperature(20)
      .with_all_sectors(SectorType::SEC_LAND)
      .build();
  TestPlanetBuilder(em, db, 1, PlanetType::MARS, Coordinates{5, 5}, 2)
      .named("Ares")
      .build();

  GB::creator::EnrollmentService service(em);

  // 1. seed_deity MC/DC validation on is_valid_login_token
  // - empty password
  test::expect_throws<std::invalid_argument>(
      [&]() { service.seed_deity("", "govpass"); });
  // - password with space
  test::expect_throws<std::invalid_argument>(
      [&]() { service.seed_deity("god pass", "govpass"); });
  // - empty gov_password
  test::expect_throws<std::invalid_argument>(
      [&]() { service.seed_deity("godpass", ""); });
  // - gov_password with space
  test::expect_throws<std::invalid_argument>(
      [&]() { service.seed_deity("godpass", "gov pass"); });

  // 2. Valid seed_deity("godpass", "govpass") creates Player 1
  service.seed_deity("godpass", "govpass");

  const auto* deity = em.peek_race(player_t{1});
  test::expect_true(deity != nullptr);
  if (deity) {
    test::expect_eq(deity->Playernum, player_t{1});
    test::expect_eq(deity->name, std::string("Deity"));
    test::expect_true(deity->God);
    test::expect_false(deity->Guest);
    test::expect_false(deity->Gov_ship.has_value());
    test::expect_eq(deity->leader().deflevel, ScopeLevel::LEVEL_UNIV);
    test::expect_eq(deity->leader().homesystem, starnum_t{0});
    test::expect_eq(deity->leader().homeplanetnum, planetnum_t{0});
  }

  // Block 1 and Power 1 created
  const auto* blk = em.peek_block(1);
  test::expect_true(blk != nullptr);
  test::expect_eq(blk->name(), std::string("Deity"));
  const auto* pwr = em.peek_power(1);
  test::expect_true(pwr != nullptr);

  // 3. Subsequent enroll_player call enrolls mortal race as player 2 with God
  // == false
  GB::creator::RaceEnrollmentSpec mortal_spec{
      .name = "Terrans",
      .password = "mortalpass",
      .home_planet_type = PlanetType::EARTH,
      .preferred_sector = SectorType::SEC_LAND,
      .is_god = false,
      .mass = 1.0,
      .birthrate = 0.5,
      .fighters = 5,
      .iq = 100,
      .number_sexes = 1,
      .metabolism = 1.0,
      .sector_compatibilities = {.land = 1.0, .plated = 1.0},
      .likesbest = SectorType::SEC_LAND,
  };

  auto result = service.enroll_player(mortal_spec);
  test::expect_true(result.success);
  test::expect_eq(result.player_num, player_t{2});

  const auto* mortal = em.peek_race(player_t{2});
  test::expect_true(mortal != nullptr);
  if (mortal) {
    test::expect_eq(mortal->name, std::string("Terrans"));
    test::expect_false(mortal->God);
  }

  std::println(std::cout, "  ✓ seed_deity and mortal enrollment verified");
}

void test_enroll_unbounded_players() {
  std::println(std::cout, "Test: Unbounded player enrollment beyond 64");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  Universe us{};
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  RaceRepository races(store);
  for (int i = 1; i <= 64; ++i) {
    Race r{};
    r.Playernum = i;
    r.name = std::format("Race{}", i);
    races.save(r);
  }

  EntityManager em(db);
  TestStarBuilder(em, db, "Sol", 1).build();
  TestPlanetBuilder(em, db, 1, PlanetType::MARS, Coordinates{5, 5}, 1)
      .named("Ares")
      .build();
  TestPlanetBuilder(em, db, 1, PlanetType::EARTH, Coordinates{5, 5}, 2)
      .named("Terra")
      .with_temperature(20)
      .with_all_sectors(SectorType::SEC_LAND)
      .build();

  GB::creator::EnrollmentService service(em);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Empire65",
      .password = "secret",
      .home_planet_type = PlanetType::EARTH,
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_true(result.success);
  test::expect_eq(result.player_num, player_t{65});

  std::println(std::cout, "  ✓ Unbounded player enrollment passed");
}

void test_enroll_no_free_planet_type() {
  std::println(std::cout, "Test: No free home planet type rejection");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);

  Universe us{};
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  EntityManager em(db);
  TestStarBuilder(em, db, "Sol", 1).build();
  TestPlanetBuilder(em, db, 1, PlanetType::MARS, Coordinates{10, 10}, 1)
      .named("MarsPlanet")
      .build();

  GB::creator::EnrollmentService service(em);

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

  Universe us{};
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  EntityManager em(db);

  // Star 1: Inhabited -> skip
  TestStarBuilder(em, db, "Star1", 1)
      .with_inhabited(player_t{1})
      .with_planet_names({"P1", "P2"})
      .build();

  // Star 2: Only 1 planet -> skip
  TestStarBuilder(em, db, "Star2", 2).with_planet_names({"P1"}).build();

  // Star 3: 2 planets, candidate Earth planet at pnum 2 (valid)
  TestStarBuilder(em, db, "Star3", 3).build();
  TestPlanetBuilder(em, db, 3, PlanetType::MARS, Coordinates{10, 10}, 1)
      .named("P1")
      .build();
  TestPlanetBuilder(em, db, 3, PlanetType::EARTH, Coordinates{10, 10}, 2)
      .named("P2")
      .with_temperature(20)
      .build();

  // Star 4: 2 planets, candidate Earth planet at pnum 1 (valid)
  TestStarBuilder(em, db, "Star4", 4).build();
  TestPlanetBuilder(em, db, 4, PlanetType::EARTH, Coordinates{10, 10}, 1)
      .named("P1")
      .with_temperature(15)
      .build();
  TestPlanetBuilder(em, db, 4, PlanetType::MARS, Coordinates{10, 10}, 2)
      .named("P2")
      .build();

  // Star 5: 2 planets, candidate Gas Giant at pnum 2 (cold: rtemp = -80)
  TestStarBuilder(em, db, "Star5", 5).build();
  TestPlanetBuilder(em, db, 5, PlanetType::MARS, Coordinates{10, 10}, 1)
      .named("P1")
      .build();
  TestPlanetBuilder(em, db, 5, PlanetType::GASGIANT, Coordinates{10, 10}, 2)
      .named("P2")
      .with_temperature(-80)
      .build();

  // Star 6: 2 planets, cryogenic Iceball at pnum 1 (rtemp = -120), hot Desert
  // at pnum 2 (rtemp = 150)
  TestStarBuilder(em, db, "Star6", 6).build();
  TestPlanetBuilder(em, db, 6, PlanetType::ICEBALL, Coordinates{10, 10}, 1)
      .named("P1")
      .with_temperature(-120)
      .build();
  TestPlanetBuilder(em, db, 6, PlanetType::DESERT, Coordinates{10, 10}, 2)
      .named("P2")
      .with_temperature(150)
      .build();

  GB::creator::EnrollmentService service(em);

  // Test 1: Given order [1, 2, 4, 3, 5, 6], should skip 1 and 2, and select
  // Star 4 (first valid candidate in order)
  std::vector<starnum_t> order1 = {1, 2, 4, 3, 5, 6};
  auto res1 = service.find_suitable_planet(PlanetType::EARTH, order1);
  test::expect_true(res1.has_value());
  if (!res1) return;
  test::expect_eq(res1->first, starnum_t{4});
  test::expect_eq(res1->second, planetnum_t{1});

  // Test 2: Given order [1, 2, 3, 4, 5, 6], should skip 1 and 2, and select
  // Star 3 (first valid candidate in order)
  std::vector<starnum_t> order2 = {1, 2, 3, 4, 5, 6};
  auto res2 = service.find_suitable_planet(PlanetType::EARTH, order2);
  test::expect_true(res2.has_value());
  if (!res2) return;
  test::expect_eq(res2->first, starnum_t{3});
  test::expect_eq(res2->second, planetnum_t{2});

  // Test 3: Gas Giant enrollment regression test (cold gas giant at -80C)
  auto res_gas = service.find_suitable_planet(PlanetType::GASGIANT, order2);
  test::expect_true(res_gas.has_value());
  if (!res_gas) return;
  test::expect_eq(res_gas->first, starnum_t{5});
  test::expect_eq(res_gas->second, planetnum_t{2});

  // Test 4: Cryogenic Iceball enrollment (cold iceball at -120C)
  auto res_ice = service.find_suitable_planet(PlanetType::ICEBALL, order2);
  test::expect_true(res_ice.has_value());
  if (!res_ice) return;
  test::expect_eq(res_ice->first, starnum_t{6});
  test::expect_eq(res_ice->second, planetnum_t{1});

  // Test 5: Hot Desert enrollment (warm desert world at 150C)
  auto res_desert = service.find_suitable_planet(PlanetType::DESERT, order2);
  test::expect_true(res_desert.has_value());
  if (!res_desert) return;
  test::expect_eq(res_desert->first, starnum_t{6});
  test::expect_eq(res_desert->second, planetnum_t{2});

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

  Universe us{};
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  EntityManager em(db);

  // Star 1 has 2 planets: Planet 1 is MARS, Planet 2 is GASGIANT
  TestStarBuilder(em, db, "Sol", 1).build();
  TestPlanetBuilder(em, db, 1, PlanetType::MARS, Coordinates{5, 5}, 1)
      .named("Ares")
      .build();
  TestPlanetBuilder(em, db, 1, PlanetType::GASGIANT, Coordinates{5, 5}, 2)
      .named("Jupiter")
      .with_temperature(-80)
      .with_all_sectors(SectorType::SEC_GAS)
      .build();

  GB::creator::EnrollmentService service(em);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Jovians",
      .password = "secret",
      .home_planet_type = PlanetType::GASGIANT,
      .preferred_sector = SectorType::SEC_GAS,
      .is_god = true,
      .mass = 1.0,
      .birthrate = 0.6,
      .fighters = 5,
      .iq = 140,
      .number_sexes = 1,
      .metabolism = 1.0,
      .sector_compatibilities = {.gas = 1.0, .plated = 0.0},
      .likesbest = SectorType::SEC_GAS,
  };

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
    test::expect_false(enrolled_race->God);
  }

  const auto* star = em.peek_star(1);
  test::expect_true(star != nullptr);
  if (star) {
    test::expect_true(star->is_explored_by(player_t{1}));
    test::expect_true(star->is_inhabited_by(player_t{1}));
  }

  const auto* planet = em.peek_planet(1, 2);
  test::expect_true(planet != nullptr);
  if (planet) {
    test::expect_gt(planet->popn(), 0);
  }
  test::expect_true(enrolled_race->Gov_ship.has_value());
  const auto* gov_ship = em.peek_ship(*enrolled_race->Gov_ship);
  test::expect_true(gov_ship != nullptr);
  if (gov_ship) {
    test::expect_eq(gov_ship->storbits(), starnum_t{1});
    test::expect_eq(gov_ship->pnumorbits(), planetnum_t{2});
  }

  std::println(std::cout, "  ✓ enroll_valid_race completed successfully");
}

}  // namespace

int main() {
  test_seed_deity_and_mortal_enrollment();
  test_enroll_unbounded_players();
  test_enroll_no_free_planet_type();
  test_find_suitable_planet_deterministic_search();
  test_enroll_valid_race_success();

  std::println(std::cout, "\n✅ All enroll tests passed!");
  return 0;
}
