// SPDX-License-Identifier: Apache-2.0

/// \file autoreport_test.cc
/// \brief Unit tests for autoreport command and database persistence.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_autoreport_dispatch() {
  std::println(std::cout, "Test: autoreport command dispatch and persistence");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Happy path: toggle autoreport ON at planet scope without args
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"autoreport"});
  test::expect_contains(g.out.str(), "has been set");

  // Verify persistence in DB
  ctx.em.clear_cache();
  const auto* p_on = ctx.em.peek_planet(1, 1);
  test::expect_ne(p_on, nullptr);
  test::expect_eq(p_on->info(player_t{1}).autorep, TELEG_MAX_AUTO);
  std::println(std::cout, "    ✓ Autoreport toggled ON and persisted");

  // 2. Happy path: toggle autoreport OFF at planet scope without args
  g.out.str("");
  ctx.assert_dispatch_success(g, {"autoreport"});
  test::expect_contains(g.out.str(), "has been unset");

  ctx.em.clear_cache();
  const auto* p_off = ctx.em.peek_planet(1, 1);
  test::expect_ne(p_off, nullptr);
  test::expect_eq(p_off->info(player_t{1}).autorep, 0);
  std::println(std::cout, "    ✓ Autoreport toggled OFF and persisted");

  // 3. Happy path: toggle autoreport with explicit planet argument from star
  // scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"autoreport", "Earth"});
  test::expect_contains(g.out.str(), "has been set");

  ctx.em.clear_cache();
  const auto* p_arg = ctx.em.peek_planet(1, 1);
  test::expect_ne(p_arg, nullptr);
  test::expect_eq(p_arg->info(player_t{1}).autorep, TELEG_MAX_AUTO);
  std::println(std::cout,
               "    ✓ Autoreport toggled with explicit planet argument");

  // 4. Star scope without args rejected
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"autoreport"});
  test::expect_contains(g.out.str(), "Scope must be a planet.");
  std::println(std::cout, "    ✓ Star scope without planet argument rejected");

  // 5. Explicit argument resolving to non-planet rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"autoreport", "/Sol"});
  test::expect_contains(g.out.str(), "Scope must be a planet.");
  std::println(std::cout, "    ✓ Non-planet target argument rejected");

  // 6. Invalid number of arguments (> 2) rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"autoreport", "1", "extra_arg"});
  test::expect_contains(g.out.str(), "Invalid number of arguments.");
  std::println(std::cout, "    ✓ Extra arguments rejected");

  // 7. Command matrix validation (roles, guests, governor, scopes)
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  TestCommandMatrix(ctx, "autoreport")
      .with_valid_argv({"autoreport"})
      .with_invalid_argv({"autoreport", "1", "extra"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_autoreport_dispatch();
  std::println(std::cout, "\n✅ All autoreport tests passed!");
  return 0;
}
