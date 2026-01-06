// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

int main() {
  TestContext ctx;

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].active = true;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test quit command
  command_t quit_argv = {"quit"};
  GB::commands::quit(quit_argv, g);

  std::string output = g.out.str();
  assert(output.find("Goodbye!") != std::string::npos);
  assert(g.disconnect_requested());

  std::println("✓ quit_test passed!");
  return 0;
}
