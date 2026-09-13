// SPDX-License-Identifier: Apache-2.0

/// \file dump_test.cc
/// \brief Unit tests for dump command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_dump_happy_paths() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Mark Earth and Sol as unvisited by Klingons to test exploration data
  // transfer
  ctx.em.mutate_planet(0, 0,
                       [](Planet& p) { p.info(player_t{2}).explored = 0; });
  ctx.em.mutate_star(0, [](Star& s) { s.explored().reset(player_t{2}); });

  // Mark Star 2 as unexplored by Federation to test skipping unexplored stars
  ctx.em.mutate_star(2, [](Star& s) { s.explored().reset(player_t{1}); });
  // Mark Planet (1, 0) as unexplored by Federation to test skipping unexplored
  // planets
  ctx.em.mutate_planet(1, 0,
                       [](Planet& p) { p.info(player_t{1}).explored = false; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Dump exploration data (10 AP deducted via FixedStar)
  ctx.assert_dispatch_success(g, {"dump", "Klingons"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");

  const auto* p_after = ctx.em.peek_planet(0, 0);
  test::expect_ne(p_after, nullptr);
  test::expect_true(p_after->info(player_t{2}).explored);

  const auto* s_after = ctx.em.peek_star(0);
  test::expect_ne(s_after, nullptr);
  test::expect_true(s_after->is_explored_by(player_t{2}));

  // Verify persistence after clearing cache
  ctx.em.clear_cache();
  const auto* p_persisted = ctx.em.peek_planet(0, 0);
  test::expect_ne(p_persisted, nullptr);
  test::expect_true(p_persisted->info(player_t{2}).explored);
}

void test_dump_insufficient_ap() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Set Star AP to 5 (< 10)
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"dump", "Klingons"});
  test::expect_contains(g.out.str(), "You don't have 10 action points there.");
}

void test_dump_role_rejections() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Guest race rejection
  {
    ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });

    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 2, 0);
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    ctx.assert_dispatch_rejected(g, {"dump", "Federation"});
    test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  }

  // 2. Leader-only rejection (Governor > 0)
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 1);
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    ctx.assert_dispatch_rejected(g, {"dump", "Klingons"});
    test::expect_contains(g.out.str(),
                          "Only the leader (Governor 0) may use this command.");
  }
}

void test_dump_domain_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"dump"});
  test::expect_contains(g.out.str(), "Syntax: dump <player> [<place> ...]");

  // 2. Invalid player name
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dump", "NonExistentPlayer"});
  test::expect_contains(g.out.str(), "No such player");
}

void test_dump_specific_places() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Reset explored status on recipient for further tests
  ctx.em.mutate_planet(0, 0,
                       [](Planet& p) { p.info(player_t{2}).explored = 0; });
  ctx.em.mutate_star(0, [](Star& s) { s.explored().reset(player_t{2}); });

  // 1. Dump specific star (/Sol)
  ctx.assert_dispatch_success(g, {"dump", "Klingons", "/Sol"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");
  test::expect_true(ctx.em.peek_planet(0, 0)->info(player_t{2}).explored);
  test::expect_true(ctx.em.peek_star(0)->is_explored_by(player_t{2}));

  // Reset explored status on recipient for further tests
  ctx.em.mutate_planet(0, 0,
                       [](Planet& p) { p.info(player_t{2}).explored = 0; });
  ctx.em.mutate_star(0, [](Star& s) { s.explored().reset(player_t{2}); });

  // 2. Dump specific planet (/Sol/Earth)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dump", "Klingons", "/Sol/Earth"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");
  test::expect_true(ctx.em.peek_planet(0, 0)->info(player_t{2}).explored);

  // 3. Dump with universe, ship, and invalid scopes (safely ignored)
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"dump", "Klingons", "/", "#100", "/NonExistentStar"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");
}

void test_dump_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  TestCommandMatrix(ctx, "dump")
      .with_valid_argv({"dump", "Klingons"})
      .with_invalid_argv({"dump", "NonExistentPlayer"})
      .with_valid_scope(ScopeLevel::LEVEL_STAR)
      .with_expected_star_ap(10)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_dump_matrix();
  test_dump_happy_paths();
  test_dump_specific_places();
  test_dump_insufficient_ap();
  test_dump_role_rejections();
  test_dump_domain_errors();

  std::println(std::cout, "✓ dump_test passed!");
  return 0;
}
