// SPDX-License-Identifier: Apache-2.0

/// \file doplanet_test.cc
/// \brief Unit tests for doplanet turn loop, ground ship movement,
/// terraforming, resource recovery, and exploration island discovery.

import dallib;
import gblib;
import test;
import std;

namespace {

Race createTestRace(player_t playernum = player_t{1}) {
  Race race{};
  race.Playernum = playernum;
  race.metabolism = 1.0;
  race.birthrate = 0.1;
  race.number_sexes = 2;
  race.fertilize = 10;
  race.adventurism = 0.5;
  race.likesbest = SectorType::SEC_LAND;
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    race.likes[i] = 0.8;
  }
  race.likes[SectorType::SEC_PLATED] = 1.0;
  return race;
}

Planet createTestPlanet() {
  Planet planet(PlanetType::EARTH);
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.slaved_to() = 0;
  planet.conditions(TOXIC) = 0;
  planet.conditions(RTEMP) = 50;
  planet.conditions(TEMP) = 50;
  for (int i = 1; i <= MAXPLAYERS; i++) {
    planet.info(player_t{i}).tax = 10;
    planet.info(player_t{i}).mob_set = 0;
    planet.info(player_t{i}).resource = 0;
    planet.info(player_t{i}).autorep = 0;
  }
  return planet;
}

Star createTestStar() {
  star_struct star_data{};
  star_data.name = "TestStar";
  star_data.star_id = 0;
  star_data.stability = 50;
  star_data.nova_stage = 0;
  star_data.temperature = 100;
  star_data.pnames.push_back("TestPlanet");
  return Star(star_data);
}

void test_moveship_onplanet() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Planet planet = createTestPlanet();

  ship_struct sdata{
      .owner = player_t{1},
      .land_coords = {5, 5},
      .special = TerraformData{.index = 0},
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.shipclass() = "2222";

  // Move once: y should increase from 5 to 6, and bounced should NOT flip order
  bool moved = moveship_onplanet(ship, planet, em);
  test::expect_true(moved);
  test::expect_eq(ship.land_coords().y, 6);
  test::expect_eq(ship.shipclass()[0], '2');
}

void test_terraform_and_plow() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Planet planet = createTestPlanet();
  SectorMap smap(planet, true);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .land_coords = {2, 2},
      .max_crew = 100,
      .popn = 100,
      .special = TerraformData{.index = 0},
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.shipclass() = "2";

  smap.get(2, 3).set_condition(SectorType::SEC_DESERT);

  terraform(ship, planet, smap, em);

  test::expect_eq(smap.get(2, 3).get_condition(), SectorType::SEC_LAND);
}

void test_do_recover() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race r1 = createTestRace(player_t{1});
  Race r2 = createTestRace(player_t{2});
  Race r3 = createTestRace(player_t{3});
  setbit(r1.allied, player_t{2});
  setbit(r2.allied, player_t{1});

  RaceRepository races(store);
  races.save(r1);
  races.save(r2);
  races.save(r3);

  Star star = createTestStar();
  Planet planet = createTestPlanet();

  planet.info(player_t{3}).resource = 100;
  planet.info(player_t{3}).destruct = 50;

  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{2}).numsectsowned = 5;

  do_recover(em, star, planet);

  test::expect_eq(planet.info(player_t{1}).resource +
                      planet.info(player_t{2}).resource,
                  100);
  test::expect_eq(planet.info(player_t{1}).destruct +
                      planet.info(player_t{2}).destruct,
                  50);
  test::expect_eq(planet.info(player_t{3}).resource, 0);
  test::expect_eq(planet.info(player_t{3}).destruct, 0);
}

void test_doplanet_full_cycle() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap initial_smap(planet, true);
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      auto& s = initial_smap.get(x, y);
      s.set_x(x);
      s.set_y(y);
      s.set_owner(1);
      s.set_popn_exact(100);
      s.set_efficiency_bounded(50);
      s.set_fert(50);
      s.set_resource(10);
      s.set_condition(SectorType::SEC_LAND);
    }
  }
  SectorRepository sectors(store);
  sectors.save_map(initial_smap);

  TurnStats stats{};
  stats.Compat[0] = 1.0;

  int result = doplanet(em, star, planet, stats);
  test::expect_ne(result, 0);

  test::expect_gt(planet.popn(), 0);
  test::expect_eq(planet.info(player_t{1}).numsectsowned, 100);
}

void test_exploration_island_discovery() {
  seed_rand(123);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.likesbest = SectorType::SEC_LAND;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.expltimer() = 0;  // Trigger exploration check this cycle
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap initial_smap(planet, true);
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      auto& s = initial_smap.get(x, y);
      s.set_x(x);
      s.set_y(y);
      s.set_owner(0);
      s.clear_popn();
      s.set_condition(SectorType::SEC_SEA);
    }
  }

  // Player 1 owns one sector at (0, 0)
  auto& s_owned = initial_smap.get(0, 0);
  s_owned.set_owner(1);
  s_owned.set_popn_exact(100);
  s_owned.set_condition(SectorType::SEC_LAND);

  // Set up multiple vacant eligible candidate island sectors that have been
  // explored by player 1
  for (int x = 1; x <= 4; ++x) {
    auto& cand = initial_smap.get(x, 0);
    cand.set_condition(SectorType::SEC_LAND);
  }

  SectorRepository sectors(store);
  sectors.save_map(initial_smap);

  TurnStats stats{};
  stats.Compat[0] = 1.0;
  // Mark candidate sectors as already explored by player 1
  for (int x = 1; x <= 4; ++x) {
    stats.Sectinfo[x][0].explored = player_t{1};
  }

  int result = doplanet(em, star, planet, stats);
  test::expect_ne(result, 0);

  // Verify that an island was claimed (stats.Claims == true)
  test::expect_true(stats.Claims);
  test::expect_eq(stats.tot_captured, 1);

  // Verify that EXACTLY 1 additional sector was claimed (1 owned + 1 new island
  // = 2 total), proving that iteration stopped after claiming the first island
  test::expect_eq(planet.info(player_t{1}).numsectsowned, 2);
}

void test_64bit_production_and_stockpiles() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.metabolism = 500.0;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).resource = 100'000;
  planet.info(player_t{1}).fuel = 200'000;
  planet.info(player_t{1}).destruct = 300'000;
  planet.info(player_t{1}).crystals = 400'000;

  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet, true);
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < 10; ++x) {
      auto& sect = smap.get(x, y);
      sect.set_x(x);
      sect.set_y(y);
      if (y == 0 && x < 5) {
        sect.set_owner(1);
        sect.set_popn_exact(100);
        sect.set_efficiency_bounded(100);
        sect.set_resource(100'000);
        sect.set_condition(SectorType::SEC_LAND);
      } else {
        sect.set_owner(0);
        sect.clear_popn();
        sect.set_condition(SectorType::SEC_SEA);
      }
    }
  }
  SectorRepository sectors(store);
  sectors.save_map(smap);

  TurnStats stats{};
  stats.Compat[0] = 1.0;

  doplanet(em, star, planet, stats);

  // Verify production recorded as 64-bit resource_t (> 65,535) without 16-bit
  // overflow
  test::expect_gt(planet.info(player_t{1}).prod_res, 65'535);
  test::expect_gt(planet.info(player_t{1}).prod_fuel, 65'535);

  // Verify stockpiles accumulated as 64-bit resource_t without overflow
  test::expect_gt(planet.info(player_t{1}).resource, 165'535);
  test::expect_gt(planet.info(player_t{1}).fuel, 265'535);
  test::expect_eq(planet.info(player_t{1}).destruct, 300'000);
  test::expect_eq(planet.info(player_t{1}).crystals, 400'000);
}

}  // namespace

int main() {
  std::println(std::cout, "Running doplanet unit tests...\n");

  std::println(std::cout, "  Testing moveship_onplanet... ");
  test_moveship_onplanet();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing terraform and plow... ");
  test_terraform_and_plow();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_recover... ");
  test_do_recover();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing doplanet full cycle... ");
  test_doplanet_full_cycle();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing exploration island discovery... ");
  test_exploration_island_discovery();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing 64-bit production and stockpiles... ");
  test_64bit_production_and_stockpiles();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All doplanet tests passed!");
  return 0;
}
