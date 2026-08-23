// SPDX-License-Identifier: Apache-2.0

/// \file treasury_test.cc
/// \brief Unit tests for the treasury command.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race with governor financials
  Race race{};
  race.Playernum = 1;
  race.name = "Bankers";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].money = 50000;
  race.governor[0].income = 1000;
  race.governor[0].profit_market = 250;
  race.governor[0].maintain = 300;
  race.governor[0].cost_tech = 150;
  race.governor[0].cost_market = 50;

  RaceRepository races(store);
  races.save(race);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test treasury output at universe level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"treasury"});
  std::string out = g.out.str();
  assert(out.contains("Income"));
  assert(out.contains("Costs"));
  assert(out.contains("1250"));  // Total income: 1000 + 250
  assert(out.contains("500"));   // Total costs: 300 + 150 + 50
  assert(out.contains("You have: 50000"));

  std::println(
      std::cout,
      "    ✓ Treasury command succeeded and printed correct financial summary");
  std::println(std::cout, "treasury_test passed!");
  return 0;
}
