// SPDX-License-Identifier: Apache-2.0

/// \file production_test.cc
/// \brief Test production command functionality and reporting via
/// CommandDescriptor.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Terrans", 100.0, false, player_t{1})
      .add_star("Sol", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH, "Earth");

  ctx.em.mutate_star(0,
                     [](Star& star) { star.mark_inhabited_by(player_t{1}); });

  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).explored = 1;
    planet.info(player_t{1}).numsectsowned = 10;
    planet.info(player_t{1}).prod_res = 100;
    planet.info(player_t{1}).prod_fuel = 50;
    planet.info(player_t{1}).prod_dest = 20;
    planet.info(player_t{1}).prod_crystals = 5;
    planet.info(player_t{1}).est_production = 175.0;
  });
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

  // 2. Production report for specific location
  ctx.assert_dispatch_success(g, {"production", "/Sol"});
  test::expect_contains(g.out.str(), "Production Report");
  test::expect_contains(g.out.str(), "Sol/Eart");

  // 3. Bad location warning
  ctx.assert_dispatch_success(g, {"production", "/InvalidStar"});
  test::expect_contains(g.out.str(), "Bad location");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_production_dispatch();

  std::println(std::cout, "✓ production_test passed!");
  return 0;
}
