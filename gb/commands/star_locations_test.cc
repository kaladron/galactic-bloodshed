// SPDX-License-Identifier: Apache-2.0

/// \file star_locations_test.cc
/// \brief Unit tests for stars (star_locations) command

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

void test_stars_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.lastx[1] = 0.0;
  g.lasty[1] = 0.0;

  // 1. Happy path: stars without distance argument (lists all stars)
  ctx.assert_dispatch_success(g, {"stars"});
  std::string output = g.out.str();
  test::expect_contains(output, "Sol");
  test::expect_contains(output, "Vega");
  std::println(std::cout, "    ✓ stars listed all stellar positions");

  // 2. Happy path: stars with radius filter (should only include Sol)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "100"});
  output = g.out.str();
  test::expect_contains(output, "Sol");
  test::expect_false(output.contains("Vega"));
  std::println(std::cout, "    ✓ stars radius filter matched proximate star");

  // 3. Radius filter matching no stars if player is far away
  g.lastx[1] = 10000.0;
  g.lasty[1] = 10000.0;
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "10"});
  test::expect_contains(g.out.str(),
                        "No stars found within specified distance.");
  std::println(std::cout, "    ✓ stars handled empty search radius cleanly");
}

}  // namespace

int main() {
  test_stars_dispatch();

  std::println(std::cout, "All stars tests passed!");
  return 0;
}
