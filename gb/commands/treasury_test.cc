// SPDX-License-Identifier: Apache-2.0

/// \file treasury_test.cc
/// \brief Unit tests for the treasury command.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx).add_race("Bankers", 100.0, false, player_t{1});

  auto race_handle = ctx.em.get_race(1);
  race_handle->governor[0].money = 50000;
  race_handle->governor[0].income = 1000;
  race_handle->governor[0].profit_market = 250;
  race_handle->governor[0].maintain = 300;
  race_handle->governor[0].cost_tech = 150;
  race_handle->governor[0].cost_market = 50;
}

void test_treasury_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test treasury output at universe level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"treasury"});
  test::expect_contains(g.out.str(), "Income");
  test::expect_contains(g.out.str(), "Costs");
  test::expect_contains(g.out.str(), "1250");  // Total income: 1000 + 250
  test::expect_contains(g.out.str(), "500");   // Total costs: 300 + 150 + 50
  test::expect_contains(g.out.str(), "You have: 50000");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_treasury_dispatch();

  std::println(std::cout, "✓ treasury_test passed!");
  return 0;
}
