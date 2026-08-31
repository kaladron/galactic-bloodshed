// SPDX-License-Identifier: Apache-2.0

/// \file schedule_test.cc
/// \brief Unit tests for schedule command and scheduling engine.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_schedule_display() {
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
}

void test_do_next_thing_branching() {
  TestContext ctx;
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "Schedulers";
  race.Guest = false;
  race.governor[0].active = true;
  RaceRepository races(store);
  races.save(race);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  // Case A: nsegments_done < segments -> advances segment
  ServerState state{};
  state.id = 1;
  state.update_time_minutes = 30;
  state.segments = 5;
  state.nsegments_done = 2;
  state.next_update_time = std::time(nullptr) + 1800;
  state.next_segment_time = std::time(nullptr) + 360;
  ServerStateRepository states(store);
  states.save(state);

  auto& registry = get_test_session_registry();
  do_next_thing(ctx.em, registry);

  const auto* state_after_seg = ctx.em.peek_server_state();
  test::expect_ne(state_after_seg, nullptr);
  test::expect_eq(state_after_seg->nsegments_done, 3);

  // Case B: nsegments_done == segments -> advances full update
  ctx.em.mutate_server_state([](ServerState& s) { s.nsegments_done = 5; });
  do_next_thing(ctx.em, registry);

  const auto* state_after_upd = ctx.em.peek_server_state();
  test::expect_ne(state_after_upd, nullptr);
  test::expect_eq(state_after_upd->nsegments_done, 1);
}

}  // namespace

int main() {
  test_schedule_display();
  test_do_next_thing_branching();
  std::println(std::cout, "✓ schedule_test passed!");
  return 0;
}
