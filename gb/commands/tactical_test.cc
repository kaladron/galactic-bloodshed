// SPDX-License-Identifier: Apache-2.0

/// \file tactical_test.cc
/// \brief Test tactical command functionality
///
/// This test verifies the standalone tactical.cc command works correctly.
/// The tactical command shows a combat display of ships and planets in the
/// current scope.

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Create a universe with combat ships and planets for tactical testing
void setup_test_universe(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.declare_war_on(2); });

  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(1).guns = 5; });

  // Ship 1: Player 1 Factory at planet scope (has sight, no guns)
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 1)
      .owned_by(1)
      .named("Factory1")
      .in_planet_orbit(1, 1)
      .build();

  // Ship 2: Player 1 Destroyer at same planet coordinates, armed,
  // moving/evading, laser focused
  {
    auto s2 = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 2)
                  .owned_by(1)
                  .named("Destroyer1")
                  .in_planet_orbit(1, 1)
                  .with_guns(guntype_t::MEDIUM, 4)
                  .with_crew(20, 0)
                  .with_speed(6)
                  .build_handle();
    s2->laser() = 1;
    s2->fire_laser() = 5;
    s2->focus() = 1;
    s2->navigate().on = 1;
    s2->protect().evade = true;
  }

  // Ship 3: Player 2 Cruiser at exact same planet coordinates (dist == 0)
  {
    auto s3 = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 3)
                  .owned_by(2)
                  .named("EnemyCruiser")
                  .with_tech(80.0)
                  .in_planet_orbit(1, 1)
                  .with_speed(4)
                  .build_handle();
    s3->navigate().on = 1;
    s3->protect().evade = true;
  }

  // Ship 4: Player 2 Landed inactive AFV on Earth
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV, 4)
      .owned_by(2)
      .named("EnemyTank")
      .with_active(false)
      .with_radiation(50)
      .landed_on(1, 1, {2, 3})
      .build();

  // Ship 5: Canister (should be excluded from tactical target rows)
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST, 5)
      .owned_by(2)
      .named("DustCanister")
      .in_planet_orbit(1, 1)
      .build();
}

/// Test tactical at planet scope - shows ships and targets orbiting the planet
void test_tactical_planet_scope() {
  std::println(std::cout, "Test: Tactical at planet scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 1);
  g_tactical.set_level(ScopeLevel::LEVEL_PLAN);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(1);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at planet scope");
  test::expect_contains(tactical_output, "Earth",
                        "Tactical at planet scope should show planet");
  // Verify same-coordinate enemy targets (dist == 0) are shown and canisters
  // excluded
  test::expect_contains(
      tactical_output, "EnemyCruiser",
      "Tactical should include enemy ship at same orbital coordinates");
  test::expect_contains(tactical_output, "EnemyTank",
                        "Tactical should include landed inactive enemy ship");
  test::expect_contains(tactical_output, "INACTIVE",
                        "Tactical should mark inactive target ship");
  test::expect_false(tactical_output.contains("DustCanister"),
                     "Tactical should exclude canisters from targets");

  std::println(
      std::cout,
      "  ✓ Planet scope produces tactical output with same-coordinate targets");
}

/// Test tactical at ship scope - shows surrounding area without duplicating the
/// scoped ship
void test_tactical_ship_scope() {
  std::println(std::cout, "Test: Tactical at ship scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 1);
  g_tactical.set_level(ScopeLevel::LEVEL_SHIP);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(1);
  g_tactical.set_shipno(2);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at ship scope");
  test::expect_contains(
      tactical_output, "Earth",
      "Tactical at ship scope should show surrounding planet");
  test::expect_contains(
      tactical_output, "EnemyCruiser",
      "Tactical at ship scope should show enemy ship in same orbit");

  // Ensure Destroyer1 header is printed only once (not duplicated)
  auto first_pos = tactical_output.find("Destroyer1");
  test::expect_true(first_pos != std::string::npos);
  auto second_pos = tactical_output.find("Destroyer1", first_pos + 1);
  test::expect_true(second_pos == std::string::npos,
                    "Scoped ship should not be reported twice");

  std::println(std::cout, "  ✓ Ship scope produces deduplicated tactical "
                          "output with surrounding area");
}

/// Test tactical at star scope - shows planets and ships in the star system
void test_tactical_star_scope() {
  std::println(std::cout, "Test: Tactical at star scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 1);
  g_tactical.set_level(ScopeLevel::LEVEL_STAR);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(1);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at star scope");
  test::expect_contains(tactical_output, "Earth",
                        "Tactical at star scope should show planet");

  std::println(std::cout, "  ✓ Star scope produces tactical output");
}

/// Test tactical explicit ship number, player filter, ship type filter, and
/// error paths
void test_tactical_explicit_ship_and_filters() {
  std::println(std::cout, "Test: Tactical explicit ship and filters");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Explicit ship + player filter: tactical #2 2
  ctx.assert_dispatch_success(g, {"tactical", "#2", "2"});
  test::expect_contains(g.out.str(), "Destroyer1");
  test::expect_contains(g.out.str(), "EnemyCruiser");

  // Ship type filter + player filter: tactical C 2 (only Cruisers owned by
  // Player 2)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tactical", "C", "2"});
  test::expect_contains(g.out.str(), "EnemyCruiser");
  test::expect_false(g.out.str().contains("EnemyTank"),
                     "Ship type filter 'C' should exclude AFVs");

  // Non-existent ship number: tactical #999
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tactical", "#999"});
  test::expect_contains(g.out.str(), "no such ship");

  // Malformed ship number: tactical #abc
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tactical", "#abc"});
  test::expect_contains(g.out.str(), "invalid ship argument");

  // Ship scope with shipno == 0
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tactical"});
  test::expect_contains(g.out.str(), "No ship is currently scoped");

  std::println(std::cout,
               "  ✓ Explicit ship and filter arguments work properly");
}

void test_tactical_scope_rejection() {
  std::println(std::cout, "Test: Tactical scope rejection at UNIV scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 1);
  g_tactical.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g_tactical, {"tactical"});
  test::expect_contains(g_tactical.out.str(), "Invalid scope for this command");
  std::println(std::cout, "  ✓ Tactical rejected at universe level");
}

}  // namespace

int main() {
  std::println(std::cout, "=== Tactical Command Test ===\n");

  test_tactical_planet_scope();
  test_tactical_ship_scope();
  test_tactical_star_scope();
  test_tactical_explicit_ship_and_filters();
  test_tactical_scope_rejection();

  std::println(std::cout, "\n✅ All tactical tests passed!");
  return 0;
}
