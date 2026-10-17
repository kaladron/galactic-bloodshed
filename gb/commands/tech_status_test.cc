// SPDX-License-Identifier: Apache-2.0

/// \file tech_status_test.cc
/// \brief Unit tests for status (tech_status) command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
  JsonStore store(ctx.db);
  PowerRepository(store).save(Power{{
      .id = 1,
      .popn = 1000,
  }});

  ctx.em.mutate_planet(1, 1, [](Planet& pl) {
    pl.info(player_t{1}).autorep = 1;
    pl.info(player_t{1}).tech_invest = 50;
    pl.info(player_t{1}).prod_res = 100;
  });
  ctx.em.mutate_planet(
      3, 1, [](Planet& pl) { pl.info(player_t{1}).explored = false; });

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1)
      .in_deep_space()
      .build();
}

void test_status_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Happy path: status without arguments (covers explored colony on Sol,
  //    uncolonized explored planet on Vega, and unexplored planet on Antares)
  ctx.assert_dispatch_success(g, {"status"});
  std::string output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth*");
  test::expect_contains(output, "1000");  // Population
  test::expect_contains(output, "50");    // Tech invest
  std::println(std::cout, "    ✓ status global colony report succeeded");

  // 2. Happy path: status with star location argument & non-controlling gov
  g.out.str("");
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth*");
  g.set_governor(governor_t{2});
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  test::expect_false(g.out.str().contains("Sol/Earth*"));
  g.set_governor(governor_t{1});
  std::println(std::cout, "    ✓ status with /Sol filter succeeded");

  // 3. Unexplored star, invalid/universe/ship locations, missing power, & JSON
  ctx.em.mutate_star(3, [](Star& s) { s.clear_explored_by(player_t{1}); });
  ctx.assert_dispatch_success(g,
                              {"status", "/Antares", "/NoSuchStar", "/", "#1"});
  test::expect_contains(g.out.str(), "Bad location `/NoSuchStar`.");
  test::expect_contains(g.out.str(), "Bad location `/`.");
  test::expect_contains(g.out.str(), "Bad location `#1`.");

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  test::expect_contains(g.out.str(), "\"type\":\"tech_status\"");

  ctx.verify_universe_invariants();

  JsonStore store(ctx.db);
  PowerRepository(store).remove(2);
  ctx.em.clear_cache();
  g.set_ui_mode(UiMode::ASCII);
  ctx.setup_game_obj(g, 2, 1);
  ctx.assert_dispatch_success(g, {"status"});
  test::expect_contains(g.out.str(), "Total Popn:  unknown");
}

}  // namespace

int main() {
  test_status_dispatch();

  std::println(std::cout, "All status tests passed!");
  return 0;
}
