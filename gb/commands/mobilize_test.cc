// SPDX-License-Identifier: Apache-2.0

/// \file mobilize_test.cc
/// \brief Unit tests for mobilize command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_mobilize_dispatch() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Mobilizers")
      .add_star("Sol", 10)
      .add_planet(1, PlanetType::EARTH, "Earth");
  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).comread = 20;
    planet.info(player_t{1}).mob_set = 20;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Query mobilization (no args)
  ctx.assert_dispatch_success(g, {"mobilize"});
  test::expect_contains(g.out.str(), "Current mobilization: 20");
  test::expect_contains(g.out.str(), "Quota: 20");
  std::println(std::cout, "    ✓ Display current mobilization succeeded");

  // 2. Set mobilization to 50%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize", "50"});
  auto saved = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved, nullptr);
  test::expect_eq(saved->info(player_t{1}).mob_set, 50);
  std::println(std::cout, "    ✓ Set mobilization to 50% succeeded");

  // 3. Set mobilization to 100%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize", "100"});
  saved = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved, nullptr);
  test::expect_eq(saved->info(player_t{1}).mob_set, 100);
  std::println(std::cout, "    ✓ Set mobilization to 100% succeeded");

  // 4. Reject illegal values (>100, <0, non-numeric)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "150"});
  test::expect_contains(g.out.str(), "Illegal value");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "-10"});
  test::expect_contains(g.out.str(), "Illegal value");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "abc"});
  test::expect_contains(g.out.str(), "Illegal value");
  std::println(std::cout, "    ✓ Illegal values rejected");

  // 5. JSON UI mode for status and set
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize"});
  test::expect_contains(g.out.str(), "\"type\":\"mobilize_status\"");
  test::expect_contains(g.out.str(), "\"quota\":100");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize", "40"});
  test::expect_contains(g.out.str(), "\"type\":\"mobilize\"");
  test::expect_contains(g.out.str(), "\"quota\":40");
  g.set_ui_mode(UiMode::ASCII);

  // 6. Reject when star lacks AP
  ctx.em.mutate_star(1, [](Star& s) { s.AP(player_t{1}) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "50"});
  test::expect_contains(g.out.str(), "You don't have 1 action points there.");

  // 7. Reject invalid scope (universal level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "50"});
  test::expect_contains(g.out.str(), "Invalid scope");
  std::println(std::cout, "    ✓ Invalid scope rejected");
}

}  // namespace

int main() {
  test_mobilize_dispatch();
  std::println(std::cout, "\n✅ All mobilize tests passed!");
  return 0;
}
