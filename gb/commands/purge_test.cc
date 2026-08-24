// SPDX-License-Identifier: Apache-2.0

/// \file purge_test.cc
/// \brief Unit tests for purge command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

// Test news purge execution by deity
void test_purge_as_god() {
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

  // Post a news item to be purged
  post(ctx.em, "Galactic bulletin\n", NewsType::ANNOUNCE);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);

  // 1. Happy Path: Deity successfully purges news
  ctx.assert_dispatch_success(g, {"purge"});
  test::expect_contains(g.out.str(), "Purged all news.");
}

// Test purge rejection for mortal player
void test_purge_as_mortal() {
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
  ctx.assert_dispatch_rejected(g, {"purge"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");
}

}  // namespace

int main() {
  test_purge_as_god();
  test_purge_as_mortal();

  std::println(std::cout, "✓ purge_test passed!");
  return 0;
}
