// SPDX-License-Identifier: Apache-2.0

/// \file toxicity_test.cc
/// \brief Unit tests for toxicity command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_toxicity_dispatch() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Toxicologists")
      .add_star("Sol", 10)
      .add_planet(1, PlanetType::EARTH, "Earth");
  ctx.em.mutate_planet(
      1, 1, [](Planet& planet) { planet.info(player_t{1}).tox_thresh = 50; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Valid threshold update
  ctx.assert_dispatch_success(g, {"toxicity", "75"});
  test::expect_contains(g.out.str(), "New threshold is: 75");
  auto saved = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved, nullptr);
  test::expect_eq(saved->info(player_t{1}).tox_thresh,
                  std::optional<std::uint32_t>{75});
  std::println(std::cout, "    ✓ Set toxicity threshold to 75 succeeded");

  // Reset threshold to 0 (disabled)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"toxicity", "0"});
  test::expect_contains(g.out.str(), "New threshold is: 0");
  saved = ctx.em.peek_planet(1, 1);
  test::expect_eq(saved->info(player_t{1}).tox_thresh, std::nullopt);
  std::println(std::cout,
               "    ✓ Reset toxicity threshold to 0 (nullopt) succeeded");

  // 2. Reject illegal values (>100, <0, non-numeric)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "150"});
  test::expect_contains(g.out.str(), "Illegal value");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "-10"});
  test::expect_contains(g.out.str(), "Illegal value");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "abc"});
  test::expect_contains(g.out.str(), "Illegal value");
  std::println(std::cout, "    ✓ Illegal values rejected");

  // 3. JSON UI mode
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"toxicity", "60"});
  test::expect_contains(g.out.str(), "\"type\":\"toxicity\"");
  test::expect_contains(g.out.str(), "\"threshold\":60");
  g.set_ui_mode(UiMode::ASCII);

  // 4. Reject missing arguments
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity"});
  test::expect_contains(g.out.str(), "Syntax: toxicity <threshold>");
  std::println(std::cout,
               "    ✓ Missing argument rejected by descriptor min_args");

  // 5. Reject invalid scope (universal level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "50"});
  test::expect_contains(g.out.str(), "Invalid scope");
  std::println(std::cout, "    ✓ Invalid scope rejected");
}

}  // namespace

int main() {
  test_toxicity_dispatch();
  std::println(std::cout, "\n✅ All toxicity tests passed!");
  return 0;
}
