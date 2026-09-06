// SPDX-License-Identifier: Apache-2.0

/// \file enrollment_test.cc
/// \brief Unit tests for canonical EnrollmentService in gb.creator module.

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import gb.creator;
import test;

namespace {

void setup_test_universe(Database& db) {
  initialize_schema(db);
  JsonStore store(db);

  universe_struct us{};
  us.id = 1;
  us.numstars = 3;
  UniverseRepository univ_repo(store);
  univ_repo.save(us);

  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);
  SectorRepository sector_repo(store);

  // Star 0: 2 planets (Earth, Gas Giant)
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.inhabited = 0;
  ss0.name = "Sol";
  ss0.pnames = {"Earth", "Jupiter"};
  Star star0(ss0);
  star_repo.save(star0);

  Planet p0_0{PlanetType::EARTH, Coordinates{5, 5}};
  p0_0.star_id() = 0;
  p0_0.planet_order() = 0;
  p0_0.conditions(RTEMP) = 20;
  p0_0.conditions(OXYGEN) = 21;
  planet_repo.save(p0_0);

  SectorMap smap0_0(p0_0);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap0_0.get(Coordinates{x, y}).set_condition(SectorType::SEC_LAND);
    }
  }
  sector_repo.save_map(smap0_0);

  Planet p0_1{PlanetType::GASGIANT, Coordinates{5, 5}};
  p0_1.star_id() = 0;
  p0_1.planet_order() = 1;
  p0_1.conditions(RTEMP) = -80;
  p0_1.conditions(METHANE) = 90;
  planet_repo.save(p0_1);

  SectorMap smap0_1(p0_1);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap0_1.get(Coordinates{x, y}).set_condition(SectorType::SEC_GAS);
    }
  }
  sector_repo.save_map(smap0_1);

  // Star 1: 1 planet (Mars) -> single planet star should be skipped
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.inhabited = 0;
  ss1.name = "Alpha";
  ss1.pnames = {"Mars"};
  Star star1(ss1);
  star_repo.save(star1);

  Planet p1_0{PlanetType::MARS, Coordinates{5, 5}};
  p1_0.star_id() = 1;
  p1_0.planet_order() = 0;
  planet_repo.save(p1_0);

  // Star 2: 2 planets (Iceball, Desert)
  star_struct ss2{};
  ss2.star_id = 2;
  ss2.inhabited = 0;
  ss2.name = "Vega";
  ss2.pnames = {"Hoth", "Dune"};
  Star star2(ss2);
  star_repo.save(star2);

  Planet p2_0{PlanetType::ICEBALL, Coordinates{5, 5}};
  p2_0.star_id() = 2;
  p2_0.planet_order() = 0;
  p2_0.conditions(RTEMP) = -120;
  planet_repo.save(p2_0);

  SectorMap smap2_0(p2_0);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap2_0.get(Coordinates{x, y}).set_condition(SectorType::SEC_ICE);
    }
  }
  sector_repo.save_map(smap2_0);

  Planet p2_1{PlanetType::DESERT, Coordinates{5, 5}};
  p2_1.star_id() = 2;
  p2_1.planet_order() = 1;
  p2_1.conditions(RTEMP) = 140;
  planet_repo.save(p2_1);

  SectorMap smap2_1(p2_1);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      smap2_1.get(Coordinates{x, y}).set_condition(SectorType::SEC_DESERT);
    }
  }
  sector_repo.save_map(smap2_1);
}

void test_first_race_requires_god() {
  std::println(std::cout, "Test: First race enrolled must have God privileges");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "MortalRace",
      .password = "pass",
      .home_planet_type = PlanetType::EARTH,
      .is_god = false,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message,
                        "The first race enrolled must have God privileges.");

  std::println(std::cout, "  ✓ First race God requirement check passed");
}

void test_max_players_rejected() {
  std::println(std::cout, "Test: Max players limit rejection");

  Database db(":memory:");
  setup_test_universe(db);
  JsonStore store(db);
  RaceRepository races(store);

  // Fill up races to MAXPLAYERS - 1
  for (int i = 1; i < MAXPLAYERS; ++i) {
    Race r{};
    r.Playernum = player_t{i};
    r.name = std::format("Empire{}", i);
    races.save(r);
  }

  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "OverflowEmpire",
      .password = "pass",
      .home_planet_type = PlanetType::EARTH,
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message, "No more allowed.");

  std::println(std::cout, "  ✓ Max players rejection passed");
}

void test_no_free_planet_rejected() {
  std::println(std::cout, "Test: No free planet of requested type rejection");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  // Request FOREST planet (none exist in test universe)
  GB::creator::RaceEnrollmentSpec spec{
      .name = "ForestDwellers",
      .password = "pass",
      .home_planet_type = PlanetType::FOREST,
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_false(result.success);
  test::expect_contains(result.message, "Didn't find any free Forest");

  std::println(std::cout, "  ✓ No free planet type rejection passed");
}

void test_enroll_first_race_god_success() {
  std::println(std::cout, "Test: Enroll first race as God on Earth");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Terrans",
      .password = "secret",
      .governor_password = "gov",
      .address = "Earthling",
      .home_planet_type = PlanetType::EARTH,
      .preferred_sector = SectorType::SEC_LAND,
      .is_god = true,
      .mass = 1.0,
      .birthrate = 1.0,
      .fighters = 10,
      .iq = 100,
      .number_sexes = 2,
      .metabolism = 1.0,
      .sector_compatibilities = {0.5, 1.0, 0.4, 0.0, 0.2, 0.8, 0.3, 0.0, 0.0},
      .likesbest = SectorType::SEC_LAND,
  };

  auto result = service.enroll_player(spec);
  test::expect_true(result.success);
  test::expect_eq(result.player_num, player_t{1});
  test::expect_eq(result.star, starnum_t{0});
  test::expect_eq(result.pnum, planetnum_t{0});
  test::expect_gt(result.gov_ship, shipnum_t{0});

  // Verify Race entity
  const auto* race = em.peek_race(player_t{1});
  test::expect_true(race != nullptr);
  if (race) {
    test::expect_eq(race->name, std::string("Terrans"));
    test::expect_true(race->God);
    test::expect_eq(race->Playernum, player_t{1});
    test::expect_eq(race->Gov_ship, result.gov_ship);
    test::expect_eq(race->number_sexes, 2u);
    test::expect_eq(race->conditions[RTEMP], 20);
    test::expect_eq(race->conditions[OXYGEN], 21);
    test::expect_eq(race->likesbest, SectorType::SEC_LAND);
    test::expect_eq(race->likes[SectorType::SEC_LAND], 1.0);
    test::expect_eq(race->governor[0].homesystem, starnum_t{0});
    test::expect_eq(race->governor[0].homeplanetnum, planetnum_t{0});
    test::expect_eq(race->governor[0].active, true);
    test::expect_eq(race->translate[player_t{1}], 100);
  }

  // Verify Capital Ship entity
  const auto* ship = em.peek_ship(result.gov_ship);
  test::expect_true(ship != nullptr);
  if (ship) {
    test::expect_eq(ship->type(), ShipType::OTYPE_GOV);
    test::expect_eq(ship->owner(), player_t{1});
    test::expect_true(ship->is_landed());
    test::expect_eq(ship->whatorbits(), ScopeLevel::LEVEL_PLAN);
    test::expect_eq(ship->storbits(), starnum_t{0});
    test::expect_eq(ship->pnumorbits(), planetnum_t{0});
    test::expect_eq(ship->land_coords(), result.capital_coords);
  }

  // Verify Planet entity
  const auto* planet = em.peek_planet(starnum_t{0}, planetnum_t{0});
  test::expect_true(planet != nullptr);
  if (planet) {
    test::expect_eq(planet->popn(), 2);
    test::expect_eq(planet->ships(), result.gov_ship);
    test::expect_eq(planet->info(player_t{1}).numsectsowned, 1u);
    test::expect_true(planet->info(player_t{1}).explored);
  }

  // Verify SectorMap
  em.with_sectormap(starnum_t{0}, planetnum_t{0}, [&](const SectorMap& smap) {
    const auto& capital_sect = smap.get(result.capital_coords);
    test::expect_eq(capital_sect.get_owner(), player_t{1});
    test::expect_eq(capital_sect.get_race(), player_t{1});
    test::expect_eq(capital_sect.get_popn(), 2);
    test::expect_eq(capital_sect.get_fert(), 100);
    test::expect_eq(capital_sect.get_eff(), 10);
  });

  // Verify Star entity
  const auto* star = em.peek_star(starnum_t{0});
  test::expect_true(star != nullptr);
  if (star) {
    test::expect_true(star->is_explored_by(player_t{1}));
    test::expect_true(star->is_inhabited_by(player_t{1}));
    test::expect_eq(star->AP(player_t{1}), 5);
  }

  std::println(std::cout, "  ✓ Enroll first race God success passed");
}

void test_enroll_second_race_mortal_success() {
  std::println(std::cout, "Test: Enroll second race as Mortal on Vega Desert");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  // First enroll Deity
  GB::creator::RaceEnrollmentSpec god_spec{
      .name = "Creator",
      .password = "godpass",
      .home_planet_type = PlanetType::EARTH,
      .is_god = true,
  };
  auto god_result = service.enroll_player(god_spec);
  test::expect_true(god_result.success);

  // Now enroll mortal race on DESERT (Star 2, Planet 1)
  GB::creator::RaceEnrollmentSpec mortal_spec{
      .name = "DesertFolk",
      .password = "mortalpass",
      .home_planet_type = PlanetType::DESERT,
      .preferred_sector = SectorType::SEC_DESERT,
      .is_god = false,
      .mass = 0.8,
      .birthrate = 1.2,
      .fighters = 12,
      .iq = 90,
      .number_sexes = 1,
      .metabolism = 1.1,
      .likesbest = SectorType::SEC_DESERT,
  };

  auto mortal_result = service.enroll_player(mortal_spec);
  test::expect_true(mortal_result.success);
  test::expect_eq(mortal_result.player_num, player_t{2});
  test::expect_eq(mortal_result.star, starnum_t{2});
  test::expect_eq(mortal_result.pnum, planetnum_t{1});

  const auto* mortal_race = em.peek_race(player_t{2});
  test::expect_true(mortal_race != nullptr);
  if (mortal_race) {
    test::expect_eq(mortal_race->name, std::string("DesertFolk"));
    test::expect_false(mortal_race->God);
    test::expect_eq(mortal_race->conditions[RTEMP], 140);
  }

  std::println(std::cout, "  ✓ Enroll second race mortal success passed");
}

void test_enroll_gas_giant_cold_success() {
  std::println(std::cout, "Test: Enroll race on cryogenic Gas Giant (-80C)");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  GB::creator::RaceEnrollmentSpec spec{
      .name = "Jovians",
      .password = "jovpass",
      .home_planet_type = PlanetType::GASGIANT,
      .preferred_sector = SectorType::SEC_GAS,
      .is_god = true,
      .number_sexes = 1,
      .likesbest = SectorType::SEC_GAS,
  };

  auto result = service.enroll_player(spec);
  test::expect_true(result.success);
  test::expect_eq(result.star, starnum_t{0});
  test::expect_eq(result.pnum, planetnum_t{1});

  const auto* race = em.peek_race(player_t{1});
  test::expect_true(race != nullptr);
  if (race) {
    test::expect_eq(race->conditions[RTEMP], -80);
    test::expect_eq(race->conditions[METHANE], 90);
    test::expect_eq(race->likesbest, SectorType::SEC_GAS);
  }

  std::println(std::cout, "  ✓ Enroll gas giant cold success passed");
}

void test_enroll_explicit_capital_coords() {
  std::println(std::cout, "Test: Enroll with explicit capital coordinates");

  Database db(":memory:");
  setup_test_universe(db);
  EntityManager em(db);
  GB::creator::EnrollmentService service(em, db);

  Coordinates explicit_coords{3, 4};
  GB::creator::RaceEnrollmentSpec spec{
      .name = "Terrans",
      .password = "pass",
      .home_planet_type = PlanetType::EARTH,
      .capital_coords = explicit_coords,
      .is_god = true,
  };

  auto result = service.enroll_player(spec);
  test::expect_true(result.success);
  test::expect_eq(result.capital_coords, explicit_coords);

  const auto* ship = em.peek_ship(result.gov_ship);
  test::expect_true(ship != nullptr);
  if (ship) {
    test::expect_eq(ship->land_coords(), explicit_coords);
  }

  em.with_sectormap(result.star, result.pnum, [&](const SectorMap& smap) {
    test::expect_eq(smap.get(explicit_coords).get_owner(), player_t{1});
  });

  std::println(std::cout, "  ✓ Explicit capital coordinates passed");
}

}  // namespace

int main() {
  test_first_race_requires_god();
  test_max_players_rejected();
  test_no_free_planet_rejected();
  test_enroll_first_race_god_success();
  test_enroll_second_race_mortal_success();
  test_enroll_gas_giant_cold_success();
  test_enroll_explicit_capital_coords();

  std::println(std::cout, "\n✅ All EnrollmentService unit tests passed!");
  return 0;
}
