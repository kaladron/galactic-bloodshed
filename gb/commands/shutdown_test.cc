// SPDX-License-Identifier: Apache-2.0

/// \file shutdown_test.cc
/// \brief Unit tests for @@shutdown command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

// Test shutdown execution by deity
void test_shutdown_as_god() {
  TestContext ctx;
  Race god_race{};
  god_race.Playernum = 1;
  god_race.name = "DeityRace";
  god_race.God = true;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(god_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);

  // 1. Happy Path: Deity successfully triggers shutdown
  ctx.assert_dispatch_success(g, {"@@shutdown"});
  assert(g.shutdown_requested());
  assert(g.out.str().contains("Doing shutdown."));
}

// Test shutdown rejection for mortal player
void test_shutdown_as_mortal() {
  TestContext ctx;
  Race mortal_race{};
  mortal_race.Playernum = 2;
  mortal_race.name = "MortalRace";
  mortal_race.God = false;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(mortal_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);

  // 2. Role Rejection: Mortal player is rejected
  ctx.assert_dispatch_rejected(g, {"@@shutdown"});
  assert(!g.shutdown_requested());
  assert(g.out.str().contains("Only deity can use this command."));
}

}  // namespace

int main() {
  test_shutdown_as_god();
  test_shutdown_as_mortal();

  std::println(std::cout, "✓ shutdown_test passed!");
  return 0;
}
