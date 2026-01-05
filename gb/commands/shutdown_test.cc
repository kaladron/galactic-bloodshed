// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

int main() {
  TestContext ctx;

  // Create test race with God privileges
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = true;
  race.governor[0].active = true;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test shutdown command
  command_t shutdown_argv = {"@@shutdown"};
  GB::commands::shutdown(shutdown_argv, g);

  std::string output = g.out.str();
  assert(output.find("shutdown") != std::string::npos);
  assert(g.shutdown_requested());

  std::println("✓ shutdown_test passed!");
  return 0;
}
