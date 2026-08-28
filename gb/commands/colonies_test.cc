// SPDX-License-Identifier: Apache-2.0

/// \file colonies_test.cc
/// \brief Test colonies command colonization report generation.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Terrans", 100.0, false, player_t{1})
      .add_star("Sol", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH, "Earth");

  ctx.em.mutate_race(1, [](Race& race) { race.conditions[0] = 50; });

  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).explored = 1;
    planet.info(player_t{1}).numsectsowned = 5;
    planet.popn() = 1000;
  });

  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{0, 0}).set_owner(1);
    smap.get(Coordinates{0, 0}).set_popn_exact(1000);
  });
}

void test_colonies_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Colonization report all stars
  ctx.assert_dispatch_success(g, {"colonies"});
  test::expect_contains(g.out.str(), "Colonization Report");
  test::expect_contains(g.out.str(), "Sol");

  // 2. Colonization report for specific star
  ctx.assert_dispatch_success(g, {"colonies", "/Sol"});
  test::expect_contains(g.out.str(), "Sol");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_colonies_dispatch();
  std::println(std::cout, "✓ colonies_test passed!");
  return 0;
}
