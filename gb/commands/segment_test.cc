// SPDX-License-Identifier: Apache-2.0

/// \file segment_test.cc
/// \brief Unit tests for @@segment command.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_segment_matrix() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  Race deity_race{};
  deity_race.Playernum = 1;
  deity_race.name = "DeityRace";
  deity_race.God = true;
  deity_race.governor[0].active = true;

  Race mortal_race{};
  mortal_race.Playernum = 2;
  mortal_race.name = "MortalRace";
  mortal_race.God = false;
  mortal_race.governor[0].active = true;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(deity_race);
    races.save(mortal_race);

    ServerStateRepository state_repo(store);
    ServerState state{};
    state.segments = 4;
    state.update_time_minutes = 60;
    state_repo.save(state);
  }

  // --- Case 1: Happy Path (God user runs @@segment without arg) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  test::expect_true(GB::commands::dispatch_command(g, GB::commands::segment_cmd,
                                                   {"@@segment"}));
  std::string out = g.out.str();
  test::expect_contains(out, "Starting segment movement...");
  test::expect_contains(out, "Segment completed.");

  // --- Case 1b: Happy Path with explicit segment number ---
  g.out.str("");
  test::expect_true(GB::commands::dispatch_command(g, GB::commands::segment_cmd,
                                                   {"@@segment", "2"}));
  out = g.out.str();
  test::expect_contains(out, "Starting segment movement...");
  test::expect_contains(out, "Segment completed.");

  // --- Case 2: Role Rejection (Mortal player cannot run @@segment) ---
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);
  g.out.str("");

  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::segment_cmd, {"@@segment"}));
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // --- Case 3: Domain Error (Invalid segment argument) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::segment_cmd, {"@@segment", "invalid"}));
  test::expect_contains(g.out.str(), "Invalid segment number.");

  // --- Case 4: Scope Testing (Valid in all scopes) ---
  for (auto scope : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                     ScopeLevel::LEVEL_PLAN, ScopeLevel::LEVEL_SHIP}) {
    g.set_level(scope);
    g.out.str("");
    test::expect_true(GB::commands::dispatch_command(
        g, GB::commands::segment_cmd, {"@@segment"}));
  }
}

}  // namespace

int main() {
  test_segment_matrix();
  std::println(std::cout, "✓ segment_test passed!");
  return 0;
}
