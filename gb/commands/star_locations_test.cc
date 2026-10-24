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
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_universe_center({0.0, 0.0});

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
  g.set_universe_center({10000.0, 10000.0});
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "10"});
  test::expect_contains(g.out.str(),
                        "No stars found within specified distance.");

  // 4. Domain errors: non-numeric, trailing garbage, negative radius, extra arg
  g.set_universe_center({0.0, 0.0});
  for (std::string bad : {"abc", "100abc", "-5"}) {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"stars", bad});
    test::expect_contains(g.out.str(), "Bad distance value.");
  }

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"stars", "100", "extra"});
  test::expect_contains(g.out.str(), "Syntax: stars [<radius>]");

  // 5. UiMode::JSON output
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "100"});
  test::expect_contains(g.out.str(), "\"type\":\"stars\"");
  test::expect_contains(g.out.str(), "\"name\":\"Sol\"");
  g.set_ui_mode(UiMode::ASCII);

  // 6. Matrix runner
  TestCommandMatrix(ctx, "stars")
      .with_valid_argv({"stars"})
      .with_invalid_argv({"stars", "abc"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_stars_dispatch();

  std::println(std::cout, "All stars tests passed!");
  return 0;
}
