// SPDX-License-Identifier: Apache-2.0

/// \file time_test.cc
/// \brief Unit tests for the time command.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TimeWatchers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Setup server state
  ServerState state{};
  state.id = 1;
  state.update_time_minutes = 60;
  state.segments = 4;
  state.nsegments_done = 1;
  state.next_update_time = std::time(nullptr) + 3600;
  state.next_segment_time = std::time(nullptr) + 900;

  ServerStateRepository states(store);
  states.save(state);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test time command at universe scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"time"});
  std::string out = g.out.str();
  assert(out.contains("Current time"));

  std::println(std::cout,
               "    ✓ Time command succeeded and printed current time");
  std::println(std::cout, "time_test passed!");
  return 0;
}
