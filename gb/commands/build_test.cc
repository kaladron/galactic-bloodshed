// SPDX-License-Identifier: Apache-2.0

/// \file build_test.cc
/// \brief Unit tests for build command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 500.0; });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{1}).resource = 10000;
    p.info(player_t{1}).fuel = 1000;
  });
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(100);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
  });
}

void test_build_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj for testing
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Build info query (0 AP)
  ctx.assert_dispatch_success(g, {"build", "?"}, 0);
  test::expect_contains(g.out.str(), "Default ship parameters");

  // 2. Test: Build a probe on planet (1 AP deducted dynamically)
  g.out.str("");
  // ":" = Probe
  ctx.assert_dispatch_success(g, {"build", ":", "5,5", "1"}, 1);

  // Verify planet resources were deducted
  ctx.em.clear_cache();
  const auto* planet_verify = ctx.em.peek_planet(1, 1);
  test::expect_ne(planet_verify, nullptr);
  test::expect_lt(planet_verify->info(player_t{1}).resource,
                  10000);  // Resources should be deducted

  // Verify ship was created (assigned lowest available ship ID #1)
  const auto* ship = ctx.em.peek_ship(1);
  test::expect_ne(ship, nullptr);
  test::expect_eq(ship->type(), ShipType::OTYPE_PROBE);
  test::expect_eq(ship->owner(), player_t{1});
  test::expect_eq(ship->whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship->storbits(), 1);
  test::expect_eq(ship->pnumorbits(), 1);
  test::expect_eq(ship->land_coords(), Coordinates(5, 5));
}

void test_build_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "action points");
}

void test_build_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Missing type argument at planet scope
  ctx.assert_dispatch_rejected(g, {"build"});
  test::expect_contains(g.out.str(), "Build what?");

  // 2. Missing coordinates argument at planet scope
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"build", ":"});
  test::expect_contains(g.out.str(), "Build where?");

  // 3. Test: Build with insufficient resources
  // Drain resources completely
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(player_t{1}).resource = 0; });
  g.out.str("");
  // Try to build probe with no resources
  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "You need");
}

void test_build_info_queries() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // Specific ship type query (":" = Space Probe)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "?", ":"}, 0);
  test::expect_contains(g.out.str(), "Space Probe");
  test::expect_contains(g.out.str(), "Can be constructed on planet.");

  // Invalid ship type query
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"build", "?", "9"});
  test::expect_contains(g.out.str(), "No such ship type.");
}

void test_build_from_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  shipnum_t initial_ships = ctx.em.num_ships();

  // 1. Factory building with 0 args ("build") and 2 args ("build 2")
  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(1, 1)
                             .landed_on(1, 1, {5, 5})
                             .with_resource(5000)
                             .with_crew(100, 0)
                             .with_on(true)
                             .with_build_type(ShipType::OTYPE_PROBE)
                             .build();

  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_snum(1);
  g.set_pnum(1);
  g.set_shipno(factory_id);

  // "build" with no arguments in factory scope builds 1 ship
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build"}, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 2});

  // "build 2" in factory scope builds 2 ships
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "2"}, 2);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 4});

  // 2. Shuttle building outside in star orbit ("build H 1")
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(1, 1)
                             .in_star_orbit(1)
                             .with_resource(50000)
                             .with_crew(50, 0)
                             .build();

  g.set_shipno(shuttle_id);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "H", "1"}, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 6});

  // 3. Habitat building inside hangar in universe orbit
  shipnum_t hab_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                         .owned_by(1, 1)
                         .in_deep_space()
                         .with_resource(5000)
                         .with_max_hanger(500)
                         .with_hanger(0)
                         .with_crew(100, 0)
                         .build();

  g.set_shipno(hab_id);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", ":", "1"}, 0, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 8});
}

}  // namespace

int main() {
  test_build_happy_paths();
  test_build_insufficient_ap();
  test_build_domain_errors();
  test_build_info_queries();
  test_build_from_ships();

  std::println(std::cout, "✓ build_test passed!");
  return 0;
}
