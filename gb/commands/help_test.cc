// SPDX-License-Identifier: Apache-2.0

/// \file help_test.cc
/// \brief Unit tests for help command dispatch

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_help_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  TestCommandMatrix(ctx, "help")
      .with_valid_argv({"help"})
      .with_invalid_argv({"help", "this_topic_does_not_exist"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(0)
      .run_matrix(g);
}

// Test help command dispatch with valid and invalid topics
void test_help_command_dispatch() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Happy Path: General help success
  ctx.assert_dispatch_success(g, {"help"});
  test::expect_false(g.out.str().empty());

  // 2. Happy Path: Topic help success
  g.out.str("");
  ctx.assert_dispatch_success(g, {"help", "build"});
  test::expect_contains(g.out.str(), "Finished.");

  // 3. Domain Error: Non-existent topic
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "this_topic_does_not_exist"});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");

  // 4. Domain Error: Path traversal attempt rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "../help"});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");
}

}  // namespace

int main() {
  test_help_matrix();
  test_help_command_dispatch();

  std::println(std::cout, "✓ help_command_test passed!");
  return 0;
}
