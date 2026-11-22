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

constexpr player_t p1 = 2;

void test_post_bulletin_ascii_and_json() {
  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder(ctx).add_race("Federation", 100.0, false, p1);
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(p1);
    s.mark_inhabited_by(p1);
    s.AP(p1) = 50;
  });

  ctx.em.mutate_race(p1, [](Race& r) { r.leader().name = "TestGovernor"; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);

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
                        "Federation \"TestGovernor\" [2,1]: Galactic news "
                        "bulletin\n");

  // Post from universe scope in JSON mode
  g.set_scope_univ();
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"post", "Universe", "bulletin"}, 0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("message":"Universe bulletin")");

  g.set_ui_mode(UiMode::ASCII);
  TestCommandMatrix(ctx, "post")
      .with_valid_argv({"post", "Hello"})
      .with_invalid_argv({"post"})
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_post_bulletin_ascii_and_json();

  std::println(std::cout, "✓ post_test passed!");
  return 0;
}
