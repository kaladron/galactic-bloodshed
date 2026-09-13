// SPDX-License-Identifier: Apache-2.0

/// \file distance_test.cc
/// \brief Unit tests for distance command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_distance_ships(TestContext& ctx) {
  ctx.with_standard_universe();

  // Set Earth to (60, 80) system coordinates for standard 3-4-5 Pythagorean
  // tests
  ctx.em.mutate_planet(0, 0, [](Planet& p) {
    p.set_system_coordinates(SystemCoordinates{60.0, 80.0});
  });

  // Ship 1: Player 1 at (0, 0)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 0)
      .in_star_orbit(0, UniverseCoordinates{0.0, 0.0})
      .build();

  // Ship 2: Player 1 at (30, 40) -> distance to ship 1 should be 50
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 2)
      .owned_by(1, 0)
      .in_star_orbit(0, UniverseCoordinates{30.0, 40.0})
      .build();

  // Ship 3: Player 2 (enemy)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 3)
      .owned_by(2, 0)
      .in_star_orbit(0, UniverseCoordinates{100.0, 100.0})
      .build();
}

void test_distance_dispatch() {
  TestContext ctx;
  setup_distance_ships(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args check: rejected when fewer than 3 args
  ctx.assert_dispatch_rejected(g, {"distance"});
  test::expect_contains(g.out.str(), "Syntax: distance <from> <to>");
  std::println(std::cout,
               "    ✓ distance rejected with insufficient arguments");

  // 2. Happy path: distance between two stars (0,0) and (300,400) -> 500
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Sol", "/Vega"});
  test::expect_contains(g.out.str(), "Distance = 500");
  std::println(std::cout, "    ✓ distance between stars calculated accurately");

  // 3. Happy path: alias dist
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dist", "/Sol", "/Vega"});
  test::expect_contains(g.out.str(), "Distance = 500");
  std::println(std::cout, "    ✓ dist alias succeeded");

  // 4. Happy path: distance between two ships (0,0) and (30,40) -> 50
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "#1", "#2"});
  test::expect_contains(g.out.str(), "Distance = 50");
  std::println(std::cout, "    ✓ distance between ships calculated accurately");

  // 5. Happy path: distance from planet to its host star (60,80) to (0,0) ->
  // 100
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Sol/Earth", "/Sol"});
  test::expect_contains(g.out.str(), "Distance = 100");
  std::println(std::cout,
               "    ✓ distance from planet to host star calculated accurately");

  // 6. Happy path: distance from planet to ship (60,80) to (0,0) -> 100
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Sol/Earth", "#1"});
  test::expect_contains(g.out.str(), "Distance = 100");
  std::println(std::cout,
               "    ✓ distance from planet to ship calculated accurately");

  // 7. Happy path: distance from planet to another star (60,80) to (300,400) ->
  // 400
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Sol/Earth", "/Vega"});
  test::expect_contains(g.out.str(), "Distance = 400");
  std::println(
      std::cout,
      "    ✓ cross-system distance from planet to star calculated accurately");

  // 8. Happy path: distance from ship to planet (30,40) to (60,80) -> 50
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "#2", "/Sol/Earth"});
  test::expect_contains(g.out.str(), "Distance = 50");
  std::println(std::cout,
               "    ✓ distance from ship to planet calculated accurately");

  // 9. Domain error: Foreign ship probe rejected when foreign ship is <to>
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "#1", "#3"});
  test::expect_contains(g.out.str(), "Nice try");
  std::println(
      std::cout,
      "    ✓ distance rejected query when foreign ship is destination");

  // 10. Domain error: Foreign ship probe rejected when foreign ship is <from>
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "#3", "#1"});
  test::expect_contains(g.out.str(), "Nice try");
  std::println(std::cout,
               "    ✓ distance rejected query when foreign ship is origin");

  // 11. Domain error: Bad origin scope
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "/NonExistentStar", "/Sol"});
  test::expect_true(g.out.str().contains("Bad scope") ||
                    g.out.str().contains("No such star"));
  std::println(std::cout, "    ✓ distance rejected invalid origin scope");

  // 12. Domain error: Bad destination scope
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "/Sol", "/NonExistentStar"});
  test::expect_true(g.out.str().contains("Bad scope") ||
                    g.out.str().contains("No such star"));
  std::println(std::cout, "    ✓ distance rejected invalid destination scope");

  // 13. Happy path: Cross-quadrant negative coordinates (300,400) to
  // (-300,-400) -> 1000
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Vega", "/Antares"});
  test::expect_contains(g.out.str(), "Distance = 1000");
  std::println(
      std::cout,
      "    ✓ cross-quadrant negative coordinates calculated accurately");

  // 14. Domain error: Universe root scope has no spatial coordinates
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "/", "/Sol"});
  test::expect_contains(g.out.str(), "Scope has no spatial coordinates.");
  std::println(std::cout,
               "    ✓ distance rejected scope with no spatial coordinates");
}

void test_distance_matrix() {
  TestContext ctx;
  setup_distance_ships(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  TestCommandMatrix(ctx, "distance")
      .with_valid_argv({"distance", "/Sol", "/Vega"})
      .with_invalid_argv({"distance", "/NonExistentStar", "/Sol"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(0)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_distance_matrix();
  test_distance_dispatch();

  std::println(std::cout, "All distance tests passed!");
  return 0;
}
