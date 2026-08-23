// SPDX-License-Identifier: Apache-2.0

/// \file update_test.cc
/// \brief Unit tests for @@update command.

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void test_update_matrix() {
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
    state.segments = 1;
    state.update_time_minutes = 60;
    state_repo.save(state);
  }

  // --- Case 1: Happy Path (God user runs @@update) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  assert(GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                        {"@@update"}));
  std::string out = g.out.str();
  assert(out.contains("Starting update..."));
  assert(out.contains("Update completed."));

  // --- Case 2: Role Rejection (Mortal player cannot run @@update) ---
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);
  g.out.str("");

  assert(!GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                         {"@@update"}));
  assert(g.out.str().contains("Only deity can use this command."));

  // --- Case 3: Scope Testing (Valid in all scopes) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  for (auto scope : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                     ScopeLevel::LEVEL_PLAN, ScopeLevel::LEVEL_SHIP}) {
    g.set_level(scope);
    g.out.str("");
    assert(GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                          {"@@update"}));
  }
}

}  // namespace

int main() {
  test_update_matrix();
  std::println(std::cout, "✓ update_test passed!");
  return 0;
}
