// SPDX-License-Identifier: Apache-2.0

/// \file invite_test.cc
/// \brief Unit tests for invite and uninvite commands.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "LeaderRace";
  race1.Guest = false;
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "AlienRace";
  race2.Guest = false;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create block for player 1
  block b1{};
  b1.Playernum = 1;
  b1.name = "TheAlliance";
  b1.motto = "United We Stand";
  b1.money = 0;
  b1.VPs = 0;

  BlockRepository blocks(store);
  blocks.save(b1);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "AlienRace"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);

  // 2. Non-leader (governor != 0) rejection
  ctx.setup_game_obj(g, 1, 1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "AlienRace"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  // Reset to governor 0
  ctx.setup_game_obj(g, 1, 0);

  // 3. Self-invite rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "LeaderRace"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self-invite rejection verified");

  // 4. Unknown player rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Nobody"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Unknown player rejection verified");

  // 5. Successful invite
  ctx.assert_dispatch_success(g, {"invite", "AlienRace"});
  ctx.em.clear_cache();
  const auto* b_invited = ctx.em.peek_block(1);
  test::expect_ne(b_invited, nullptr);
  test::expect_true(isset(b_invited->invite, player_t{2}));
  std::println(std::cout, "    ✓ Invite succeeded and bit set");

  // 6. Successful uninvite
  ctx.assert_dispatch_success(g, {"uninvite", "AlienRace"});
  ctx.em.clear_cache();
  const auto* b_uninvited = ctx.em.peek_block(1);
  test::expect_ne(b_uninvited, nullptr);
  test::expect_false(isset(b_uninvited->invite, player_t{2}));
  std::println(std::cout, "    ✓ Uninvite succeeded and bit cleared");

  std::println(std::cout, "invite_test passed!");
  return 0;
}
