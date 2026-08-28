// SPDX-License-Identifier: Apache-2.0

/// \file build_test.cc
/// \brief Unit tests for build command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  // Initialize database
  JsonStore store(ctx.db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.governor[0].active = true;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 500.0;  // High tech to build any ship
  race.pods = false;

  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  star_data.AP[0] = 100;
  Star star{star_data};
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a test planet with resources
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.xpos() = 0.0;
  planet.ypos() = 0.0;
  planet.info(player_t{1}).resource = 10000;  // Plenty of resources
  planet.info(player_t{1}).fuel = 1000;

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a sectormap with a sector with population for building
  SectorMap smap(planet);  // Initialize empty sectors
  smap.get(Coordinates{5, 5}).set_owner(1);
  smap.get(Coordinates{5, 5}).set_popn_exact(100);
  smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
  SectorRepository sectors_repo(store);
  sectors_repo.save_map(smap);
}

void test_build_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj for testing
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Build info query (0 AP)
  ctx.assert_dispatch_success(g, {"build", "?"}, 0);
  test::expect_contains(g.out.str(), "Default ship parameters");

  // 2. Test: Build a probe on planet (1 AP deducted dynamically)
  g.out.str("");
  // ":" = Probe
  ctx.assert_dispatch_success(g, {"build", ":", "5,5", "1"}, 1);

  // Verify planet resources were deducted
  ctx.em.clear_cache();
  const auto* planet_verify = ctx.em.peek_planet(1, 0);
  test::expect_ne(planet_verify, nullptr);
  test::expect_lt(planet_verify->info(player_t{1}).resource,
                  10000);  // Resources should be deducted

  // Verify ship was created (it should be ship #1)
  const auto* ship = ctx.em.peek_ship(1);
  test::expect_ne(ship, nullptr);
  test::expect_eq(ship->type(), ShipType::OTYPE_PROBE);
  test::expect_eq(ship->owner(), player_t{1});
  test::expect_eq(ship->whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship->storbits(), 1);
  test::expect_eq(ship->pnumorbits(), 0);
  test::expect_eq(ship->land_coords(), Coordinates(5, 5));
}

void test_build_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "action points");
}

void test_build_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"build"});
  test::expect_contains(g.out.str(),
                        "Syntax: build <type> <x,y> [count] | build ? [type]");

  // 2. Test: Build with insufficient resources
  // Drain resources completely
  ctx.em.mutate_planet(1, 0,
                       [](Planet& p) { p.info(player_t{1}).resource = 0; });
  g.out.str("");
  // Try to build probe with no resources
  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  // The build command should fail due to insufficient resources. The error is
  // written to g.out
  test::expect_contains(g.out.str(), "You need");
}

}  // namespace

int main() {
  test_build_happy_paths();
  test_build_insufficient_ap();
  test_build_domain_errors();

  std::println(std::cout, "✓ build_test passed!");
  return 0;
}
