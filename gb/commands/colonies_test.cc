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
  ctx.with_standard_universe().with_populated_planet(0, 0, 1, 1000,
                                                     Coordinates{0, 0});
  ctx.em.mutate_race(1, [](Race& race) { race.conditions[0] = 50; });
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
