// SPDX-License-Identifier: Apache-2.0

/// \file time_test.cc
/// \brief Unit tests for the time and schedule commands.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  ctx.with_standard_universe();

  const std::time_t now = std::time(nullptr);
  ctx.em.mutate_server_state([now](ServerState& state) {
    state.update_time_minutes = 60;
    state.segments = 4;
    state.nsegments_done = 1;
    state.nupdates_done = 2;
    state.record_server_start(now - 7200);
    state.record_update_completed(now - 3600, false);
    state.record_segment_completed(now - 1800);
    state.next_update_time = now + 3600;
    state.next_segment_time = now + 900;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Test time command in ASCII mode
  g.out.str("");
  ctx.assert_dispatch_success(g, {"time"});
  std::string out = g.out.str();
  test::expect_contains(out, "Server started  :");
  test::expect_contains(out, "Last Update   2 :");
  test::expect_contains(out, "Last Segment  1 :");
  test::expect_contains(out, "Current time    :");

  // 2. Test time command in JSON mode
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"time"});
  out = g.out.str();
  test::expect_contains(out, "\"type\":\"time\"");
  test::expect_contains(out, "\"start_line\":\"Server started  :");
  test::expect_contains(out, "\"update_line\":\"Last Update   2 :");
  test::expect_contains(out, "\"segment_line\":\"Last Segment  1 :");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Test schedule command when nsegments_done < segments
  g.out.str("");
  ctx.assert_dispatch_success(g, {"schedule"});
  out = g.out.str();
  test::expect_contains(out, "60 minute update intervals");
  test::expect_contains(out, "4 movement segments per update");
  test::expect_contains(out, "Next Segment  2 :");
  test::expect_contains(out, "Next Update   3 :");

  // 4. Test schedule command when nsegments_done == segments (wraps to 1) and
  // JSON mode
  ctx.em.mutate_server_state(
      [](ServerState& state) { state.nsegments_done = state.segments; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"schedule"});
  out = g.out.str();
  test::expect_contains(out, "Next Segment  1 :");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"schedule"});
  out = g.out.str();
  test::expect_contains(out, "\"type\":\"schedule\"");
  test::expect_contains(out, "\"update_time_minutes\":60");
  test::expect_contains(out, "\"segments\":4");
  test::expect_contains(out, "\"next_segment_number\":1");
  test::expect_contains(out, "\"next_update_number\":3");
  g.set_ui_mode(UiMode::ASCII);

  ctx.verify_universe_invariants();
  std::println(std::cout, "time_test passed!");
  return 0;
}
