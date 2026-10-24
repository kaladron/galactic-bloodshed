// SPDX-License-Identifier: Apache-2.0

/// \file post_test.cc
/// \brief Unit tests for the post public bulletin command.

import commands;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void test_post_bulletin_ascii_and_json() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.leader().name = "TestGovernor"; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // Missing message argument rejected by min_args
  ctx.assert_dispatch_rejected(g, {"post"});
  test::expect_contains(g.out.str(), "Syntax: post <message>");

  // Post from star scope (0 AP, recorded in NewsType::ANNOUNCE)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"post", "Galactic", "news", "bulletin"}, 0);
  test::expect_eq(g.out.str(), "");

  const auto items = ctx.em.get_news_since(NewsType::ANNOUNCE, 0);
  test::expect_false(items.empty());
  test::expect_contains(items.back().message,
                        "Federation \"TestGovernor\" [1,1]: Galactic news "
                        "bulletin\n");

  // Post from universe scope in JSON mode
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"post", "Universe", "bulletin"}, 0);
  test::expect_contains(g.out.str(), "\"type\":\"post\"");
  test::expect_contains(g.out.str(), "\"message\":\"Universe bulletin\"");

  g.set_ui_mode(UiMode::ASCII);
  TestCommandMatrix(ctx, "post")
      .with_valid_argv({"post", "Hello"})
      .with_invalid_argv({"post"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_post_bulletin_ascii_and_json();

  std::println(std::cout, "✓ post_test passed!");
  return 0;
}
