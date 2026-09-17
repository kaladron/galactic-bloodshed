// SPDX-License-Identifier: Apache-2.0

/// \file route_test.cc
/// \brief Unit tests for route command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
}

void test_route_persistence() {
  TestContext ctx;
  setup_test_world(ctx);

  // Test: Set route destination
  ctx.em.mutate_planet(0, 0, [](Planet& p) {
    p.info(player_t{1}).route[0].set = true;
    p.info(player_t{1}).route[0].dest_star = 1;
    p.info(player_t{1}).route[0].dest_planet = 0;
    p.info(player_t{1}).route[0].dest_coords = {5, 5};
    p.info(player_t{1}).route[0].load =
        CommodityManifest{.fuel = true, .resources = true};
    p.info(player_t{1}).route[0].unload = CommodityManifest{.destruct = true};
  });

  // Verify: Route was saved
  {
    const auto* saved = ctx.em.peek_planet(0, 0);
    test::expect_ne(saved, nullptr);
    test::expect_true(saved->info(player_t{1}).route[0].set);
    test::expect_eq(saved->info(player_t{1}).route[0].dest_star, starnum_t{1});
    test::expect_eq(saved->info(player_t{1}).route[0].dest_planet,
                    planetnum_t{0});
    test::expect_eq(saved->info(player_t{1}).route[0].dest_coords,
                    Coordinates(5, 5));
    test::expect_true(saved->info(player_t{1}).route[0].load.fuel);
    test::expect_true(saved->info(player_t{1}).route[0].load.resources);
    test::expect_false(saved->info(player_t{1}).route[0].load.destruct);
    test::expect_true(saved->info(player_t{1}).route[0].unload.destruct);
    std::println(std::cout, "✓ Route destination saved correctly");
  }

  // Test: Deactivate route
  ctx.em.mutate_planet(
      0, 0, [](Planet& p) { p.info(player_t{1}).route[0].set = false; });

  // Verify: Route deactivated
  {
    const auto* saved = ctx.em.peek_planet(0, 0);
    test::expect_ne(saved, nullptr);
    test::expect_false(saved->info(player_t{1}).route[0].set);
    std::println(std::cout, "✓ Route deactivation saved correctly");
  }

  // Test: Multiple routes
  ctx.em.mutate_planet(0, 0, [](Planet& p) {
    for (int i = 0; i < MAX_ROUTES; i++) {
      p.info(player_t{1}).route[i].set = true;
      p.info(player_t{1}).route[i].dest_star = 1;
      p.info(player_t{1}).route[i].dest_planet = 0;
      p.info(player_t{1}).route[i].load = CommodityManifest{.fuel = true};
    }
  });

  // Verify: All routes saved
  {
    const auto* saved = ctx.em.peek_planet(0, 0);
    test::expect_ne(saved, nullptr);
    for (int i = 0; i < MAX_ROUTES; i++) {
      test::expect_true(saved->info(player_t{1}).route[i].set);
      test::expect_true(saved->info(player_t{1}).route[i].load.fuel);
    }
    std::println(std::cout, "✓ Multiple routes saved correctly");
  }
}

void test_route_command_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"route"});
  test::expect_contains(g.out.str(), "Invalid scope for this command");

  // 2. Command dispatch happy paths at PLAN scope
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Activate route 1
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "activate"});
  test::expect_true(ctx.em.peek_planet(0, 0)->info(player_t{1}).route[0].set);

  // Set destination
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "/Vega/Vega Prime"});
  test::expect_contains(g.out.str(), "Set");

  // Set land coords
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "land", "3,3"});
  test::expect_contains(g.out.str(), "Set");

  // Set load commodities
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "load", "fr"});
  test::expect_contains(g.out.str(), "Set");

  // View routes
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route"});
  test::expect_contains(g.out.str(), "Done");

  // 3. Domain error: Bad route number (out-of-range and non-numeric)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "99"});
  test::expect_contains(g.out.str(), "Bad route number");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "abc"});
  test::expect_contains(g.out.str(), "Bad route number");

  // 4. Domain error: Bad coordinates (unparseable and out-of-bounds)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "1", "land", "invalid"});
  test::expect_contains(g.out.str(), "Bad sector coordinates");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "1", "land", "99,99"});
  test::expect_contains(g.out.str(), "Bad sector coordinates");

  // 5. Unload commodities and single route inspection
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "unload", "fdx"});
  test::expect_contains(g.out.str(), "Set");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "load", "fdrx"});
  test::expect_contains(g.out.str(), "Set");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1"});
  test::expect_contains(g.out.str(), "load: fdrx");
  test::expect_contains(g.out.str(), "unload: fdx");
  test::expect_contains(g.out.str(), "Done");

  // Inspecting an inactive route prints Done without route details
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "2"});
  test::expect_contains(g.out.str(), "Done");

  // 6. Deactivate route and verify
  g.out.str("");
  ctx.assert_dispatch_success(g, {"route", "1", "deactivate"});
  test::expect_contains(g.out.str(), "Set");
  test::expect_false(ctx.em.peek_planet(0, 0)->info(player_t{1}).route[0].set);

  // 7. Destination validation errors and unknown 4-arg subcommand
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "1", "/NoSuchStar"});
  test::expect_contains(g.out.str(), "Illegal destination");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "1", "/Vega"});
  test::expect_contains(g.out.str(), "You have to designate a planet");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"route", "1", "bogus", "fr"});
  test::expect_contains(g.out.str(), "What are you trying to do?");

  // 8. Direct handler scope guard
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  test::expect_false(GB::commands::route({"route"}, g));
  test::expect_contains(g.out.str(),
                        "You have to 'cs' to a planet to examine routes.");
}

}  // namespace

int main() {
  test_route_persistence();
  test_route_command_dispatch();

  std::println(std::cout, "All route tests passed!");
  return 0;
}
