// SPDX-License-Identifier: Apache-2.0

/// \file quit_test.cc
/// \brief Unit tests for quit command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

// Test disconnecting from the server via quit command
void test_quit_command() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Happy Path: Player requests quit
  ctx.assert_dispatch_success(g, {"quit"});
  assert(g.out.str().contains("Goodbye!"));
  assert(g.disconnect_requested());
}

}  // namespace

int main() {
  test_quit_command();

  std::println(std::cout, "✓ quit_test passed!");
  return 0;
}
