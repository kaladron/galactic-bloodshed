// SPDX-License-Identifier: Apache-2.0

/// \file pledge_test.cc
/// \brief Test pledge and unpledge commands for alliance blocks via
/// CommandDescriptor.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_pledge_and_unpledge_dispatch() {
  std::println(
      std::cout,
      "Test: pledge and unpledge command dispatch and alliance blocks");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "AllianceLeader";
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup block for player 2
  block block2{};
  block2.Playernum = 2;
  block2.name = "GalacticCoalition";
  BlockRepository blocks(store);
  blocks.save(block2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Pledge to block 2
  ctx.assert_dispatch_success(g, {"pledge", "2"});
  const auto* saved_block = ctx.em.peek_block(2);
  test::expect_ne(saved_block, nullptr);
  test::expect_true(isset(saved_block->pledge, player_t{1}));
  std::println(std::cout, "    ✓ Pledged allegiance successfully");

  // 2. Unpledge from block 2
  ctx.assert_dispatch_success(g, {"unpledge", "2"});
  saved_block = ctx.em.peek_block(2);
  test::expect_ne(saved_block, nullptr);
  test::expect_false(isset(saved_block->pledge, player_t{1}));
  std::println(std::cout, "    ✓ Unpledged successfully");

  // 3. Self pledge rejection
  ctx.assert_dispatch_rejected(g, {"pledge", "1"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self pledge rejection verified");

  // 4. Self unpledge rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "1"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self unpledge rejection verified");

  // 5. Role check: Governor cannot pledge or unpledge
  g.set_governor(1);
  ctx.assert_dispatch_rejected(g, {"pledge", "2"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "2"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout,
               "    ✓ Governor rejection verified for pledge and unpledge");

  // 6. Invalid target player rejection
  g.set_governor(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");
}

}  // namespace

int main() {
  test_pledge_and_unpledge_dispatch();
  std::println(std::cout, "\n✅ All pledge and unpledge tests passed!");
  return 0;
}
