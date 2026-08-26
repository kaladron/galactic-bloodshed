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

  auto race_handle = ctx.em.get_race(1);
  race_handle->conditions[0] = 50;

  auto planet_handle = ctx.em.get_planet(0, 0);
  planet_handle->info(player_t{1}).explored = 1;
  planet_handle->info(player_t{1}).numsectsowned = 5;
  planet_handle->popn() = 1000;

  auto smap_handle = ctx.em.get_sectormap(0, 0);
  smap_handle->get(Coordinates{0, 0}).set_owner(1);
  smap_handle->get(Coordinates{0, 0}).set_popn_exact(1000);
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
