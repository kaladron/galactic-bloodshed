// SPDX-License-Identifier: Apache-2.0

/// \file vote_test.cc
/// \brief Unit tests for vote command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void test_vote_dispatch() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create test race
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Democracy";
  race1.governor[0].active = true;
  race1.votes = false;
  RaceRepository races(store);
  races.save(race1);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Inspect current vote status (no args)
  ctx.assert_dispatch_success(g, {"vote"});
  assert(g.out.str().contains("Your vote on updates is wait"));
  assert(g.out.str().contains("Total votes = 1"));
  std::println(std::cout, "    ✓ Inspect vote status succeeded");

  // 2. Cast vote 'go'
  g.out.str("");
  ctx.assert_dispatch_success(g, {"vote", "update", "go"});
  const auto* updated_race = ctx.em.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->votes == true);
  std::println(std::cout, "    ✓ Vote update go succeeded");

  // 3. Cast vote 'wait'
  g.out.str("");
  ctx.assert_dispatch_success(g, {"vote", "update", "wait"});
  updated_race = ctx.em.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->votes == false);
  std::println(std::cout, "    ✓ Vote update wait succeeded");

  // 4. Reject invalid vote topic
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"vote", "invalid", "go"});
  assert(g.out.str().contains("No such vote"));
  std::println(std::cout, "    ✓ Invalid vote topic rejected");

  // 5. Reject invalid update choice
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"vote", "update", "maybe"});
  assert(g.out.str().contains("No such update choice"));
  std::println(std::cout, "    ✓ Invalid update choice rejected");

  // 6. Deity vote message
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"vote"});
  assert(g.out.str().contains(
      "Your vote doesn't count, however, here is the count."));
  std::println(std::cout, "    ✓ Deity vote message verified");
  g.set_god(false);

  // 7. Guest vote message
  auto race1_handle = ctx.em.get_race(1);
  race1_handle->Guest = true;
  g.out.str("");
  ctx.assert_dispatch_success(g, {"vote"});
  assert(g.out.str().contains(
      "You are not allowed to vote, but, here is the count."));
  std::println(std::cout, "    ✓ Guest vote message verified");
}

}  // namespace

int main() {
  test_vote_dispatch();
  std::println(std::cout, "\n✅ All vote tests passed!");
  return 0;
}
