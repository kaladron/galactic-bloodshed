// SPDX-License-Identifier: Apache-2.0

/// \file schedule_test.cc
/// \brief Unit tests for the schedule command.

import dallib;
import gblib;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Schedulers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Setup server state
  ServerState state{};
  state.id = 1;
  state.update_time_minutes = 30;
  state.segments = 5;
  state.nsegments_done = 2;
  state.next_update_time = std::time(nullptr) + 1800;
  state.next_segment_time = std::time(nullptr) + 360;

  ServerStateRepository states(store);
  states.save(state);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Test schedule command at universe scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"schedule"});
  std::string out = g.out.str();
  test::expect_contains(out, "30 minute update intervals");
  test::expect_contains(out, "5 movement segments per update");
  test::expect_contains(out, "Next Segment");
  test::expect_contains(out, "Next Update");

  std::println(std::cout,
               "    ✓ Schedule command succeeded and printed schedule");
  std::println(std::cout, "schedule_test passed!");
  return 0;
}
