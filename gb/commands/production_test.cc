// SPDX-License-Identifier: Apache-2.0

/// \file production_test.cc
/// \brief Test production command functionality and reporting via
/// CommandDescriptor.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 10;
    planet.info(player_t{1}).prod_res = 100;
    planet.info(player_t{1}).prod_fuel = 50;
    planet.info(player_t{1}).prod_dest = 20;
    planet.info(player_t{1}).prod_crystals = 5;
    planet.info(player_t{1}).est_production = 175.0;
  });

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1)
      .in_deep_space()
      .build();
}

void test_production_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Production report for all stars (no args)
  ctx.assert_dispatch_success(g, {"production"});
  test::expect_contains(g.out.str(), "Production Report");
  test::expect_contains(g.out.str(), "Sol/Eart");

  // 2. Planet-scoped argument filters to only that planet
  TestPlanetBuilder(ctx, 1, PlanetType::MARS)
      .named("Mars")
      .with_colony(1, 500)
      .build();
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Sol/Earth"});
  test::expect_contains(g.out.str(), "Sol/Eart");
  test::expect_false(g.out.str().contains("Sol/Mars"));

  // Star-scoped argument includes all colonies under that star
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Sol"});
  test::expect_contains(g.out.str(), "Sol/Eart");

  // Non-controlling governor filters out colonies
  g.set_governor(governor_t{2});
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Sol"});
  test::expect_false(g.out.str().contains("Sol/Eart"));
  g.set_governor(governor_t{1});

  // 3. Bad location warnings (no duplicate PlaceError text) and ship places
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/InvalidStar", "/", "#1"});
  test::expect_contains(g.out.str(), "Bad location `/InvalidStar`.");
  test::expect_contains(g.out.str(), "Bad location `/`.");
  test::expect_contains(g.out.str(), "Bad location `#1`.");
  test::expect_false(g.out.str().contains("No such star"));

  // 4. Unexplored star handling (all stars and specific planet via god)
  ctx.em.mutate_star(3, [](Star& s) { s.clear_explored_by(player_t{1}); });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production"});
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Antares/Antares Prime"});
  g.set_god(false);

  // 5. UiMode::JSON output
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Sol/Earth"});
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("name":"Earth")");
  test::expect_contains(g.out.str(), R"("resources":100)");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_production_dispatch();

  std::println(std::cout, "✓ production_test passed!");
  return 0;
}
