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

// Test help command dispatch with valid and invalid topics
void test_help_command_dispatch() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Happy Path: Topic help success
  ctx.assert_dispatch_success(g, {"help", "build"});
  test::expect_contains(g.out.str(), "Finished.");

  // 2. Domain Error: Non-existent topic
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "this_topic_does_not_exist"});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");
}

}  // namespace

int main() {
  test_help_command_dispatch();

  std::println(std::cout, "✓ help_command_test passed!");
  return 0;
}
