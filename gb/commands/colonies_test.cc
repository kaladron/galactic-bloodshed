// SPDX-License-Identifier: Apache-2.0

/// \file colonies_test.cc
/// \brief Test colonies command colonization report generation.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe()
      .with_populated_planet(1, 1, 1, 1000, Coordinates{0, 0})
      .with_populated_planet(1, 1, 2, 500, Coordinates{1, 0});
  ctx.em.mutate_race(1, [](Race& race) { race.temp = 50; });
  ctx.em.mutate_planet(1, 1,
                       [](Planet& pl) { pl.info(player_t{1}).autorep = 1; });
  ctx.em.mutate_planet(
      3, 1, [](Planet& pl) { pl.info(player_t{1}).explored = false; });
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1)
      .in_deep_space()
      .build();
}

void test_colonies_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Colonization report all stars (covers explored colony + alien co-owner,
  //    uncolonized explored planet on Vega, and unexplored planet on Antares)
  ctx.assert_dispatch_success(g, {"colonies"});
  test::expect_contains(g.out.str(), "Colonization Report");
  test::expect_contains(g.out.str(), "Sol");
  test::expect_contains(g.out.str(), "Eart*");

  // 2. Colonization report for specific star & non-controlling governor
  ctx.assert_dispatch_success(g, {"colonies", "/Sol"});
  test::expect_contains(g.out.str(), "Sol");
  g.set_governor(governor_t{2});
  ctx.assert_dispatch_success(g, {"colonies", "/Sol"});
  test::expect_false(g.out.str().contains("Earth"));
  g.set_governor(governor_t{1});

  // 3. Unexplored star and invalid/universe/ship locations
  ctx.em.mutate_star(3, [](Star& s) { s.clear_explored_by(player_t{1}); });
  ctx.assert_dispatch_success(
      g, {"colonies", "/Antares", "/NoSuchStar", "/", "#1"});
  test::expect_contains(g.out.str(), "Bad location `/NoSuchStar'.");
  test::expect_contains(g.out.str(), "Bad location `/'.");
  test::expect_contains(g.out.str(), "Bad location `#1'.");

  // 4. JSON mode
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"colonies", "/Sol"});
  test::expect_contains(g.out.str(), "\"rows\"");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_colonies_dispatch();
  std::println(std::cout, "✓ colonies_test passed!");
  return 0;
}
