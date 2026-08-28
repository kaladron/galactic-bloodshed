// SPDX-License-Identifier: Apache-2.0

/// \file vote_test.cc
/// \brief Unit tests for vote command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx).add_race("Democracy", 100.0, false, player_t{1});

  ctx.em.mutate_race(1, [](Race& r) { r.votes = false; });
}

void test_vote_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Inspect current vote status (no args)
  ctx.assert_dispatch_success(g, {"vote"});
  test::expect_contains(g.out.str(), "Your vote on updates is wait");
  test::expect_contains(g.out.str(), "Total votes = 1");

  // 2. Cast vote 'go'
  ctx.assert_dispatch_success(g, {"vote", "update", "go"});
  const auto* updated_race = ctx.em.peek_race(1);
  test::expect_true(updated_race != nullptr);
  test::expect_eq(updated_race->votes, true);

  // 3. Cast vote 'wait'
  ctx.assert_dispatch_success(g, {"vote", "update", "wait"});
  updated_race = ctx.em.peek_race(1);
  test::expect_true(updated_race != nullptr);
  test::expect_eq(updated_race->votes, false);

  // 4. Reject invalid vote topic
  ctx.assert_dispatch_rejected(g, {"vote", "invalid", "go"});
  test::expect_contains(g.out.str(), "No such vote");

  // 5. Reject invalid update choice
  ctx.assert_dispatch_rejected(g, {"vote", "update", "maybe"});
  test::expect_contains(g.out.str(), "No such update choice");

  // 6. Deity vote message
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"vote"});
  test::expect_contains(g.out.str(),
                        "Your vote doesn't count, however, here is the count.");
  g.set_god(false);

  // 7. Guest vote message
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.assert_dispatch_success(g, {"vote"});
  test::expect_contains(g.out.str(),
                        "You are not allowed to vote, but, here is the count.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_vote_dispatch();
  std::println(std::cout, "✓ vote_test passed!");
  return 0;
}
