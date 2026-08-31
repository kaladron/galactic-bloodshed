// SPDX-License-Identifier: Apache-2.0

/// \file vote_test.cc
/// \brief Unit tests for vote command

import commands;
import dallib;
import gb.entities;
import gb.services;
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

void test_unanimous_vote_lifecycle() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("MortalOne", 100.0, false, player_t{1})
      .add_race("MortalTwo", 100.0, false, player_t{2})
      .add_race("GodRace", 100.0, false, player_t{3})
      .add_race("GuestRace", 100.0, false, player_t{4});

  ctx.em.mutate_race(3, [](Race& r) { r.God = true; });
  ctx.em.mutate_race(4, [](Race& r) { r.Guest = true; });

  {
    JsonStore store(ctx.db);
    universe_struct u{};
    u.id = 1;
    u.numstars = 1;
    u.planet_count = 1;
    UniverseRepository univs(store);
    univs.save(u);

    ServerState state{};
    state.id = 1;
    state.segments = 1;
    state.nsegments_done = 1;
    state.update_time_minutes = 60;
    ServerStateRepository states(store);
    states.save(state);
  }

  auto& registry = get_test_session_registry();
  registry.clear_pending_turn();
  GameObj g1(ctx.em, registry);
  ctx.setup_game_obj(g1, 1, 0);

  GameObj g2(ctx.em, registry);
  ctx.setup_game_obj(g2, 2, 0);

  // 1. Initial inspection: both mortals are in 'wait' state
  ctx.assert_dispatch_success(g1, {"vote"});
  test::expect_contains(g1.out.str(), "Total votes = 2, Go = 0, Wait = 2");

  // 2. Player 1 votes 'go' (partial - no update triggered yet)
  ctx.assert_dispatch_success(g1, {"vote", "update", "go"});
  test::expect_true(ctx.em.peek_race(1)->votes);
  test::expect_false(ctx.em.peek_race(2)->votes);
  test::expect_false(registry.has_pending_turn());

  // 3. Player 2 votes 'go' (now unanimous across mortal races: 2/2)
  // This sets the pending turn request flag on the session registry.
  ctx.assert_dispatch_success(g2, {"vote", "update", "go"});
  test::expect_true(ctx.em.peek_race(2)->votes);
  test::expect_true(registry.has_pending_turn());

  // 4. Server tick processes the pending turn request cleanly
  if (registry.has_pending_turn()) {
    do_next_thing(ctx.em, registry);
    registry.clear_pending_turn();
  }
  test::expect_false(registry.has_pending_turn());

  // After full update execution, votes are automatically reset to false
  test::expect_false(ctx.em.peek_race(1)->votes);
  test::expect_false(ctx.em.peek_race(2)->votes);
}

}  // namespace

int main() {
  test_vote_dispatch();
  test_unanimous_vote_lifecycle();
  std::println(std::cout, "✓ vote_test passed!");
  return 0;
}
