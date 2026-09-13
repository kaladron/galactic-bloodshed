// SPDX-License-Identifier: Apache-2.0

/// \file misc_test.cc
/// \brief Unit tests for miscellaneous entity and turn helpers (adjust_morale,
/// add_to_queue, telegram_star).

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_adjust_morale() {
  Race winner{};
  winner.Playernum = 1;
  winner.name = "Victors";
  winner.morale = 100.0;

  Race loser{};
  loser.Playernum = 2;
  loser.name = "Defeated";
  loser.morale = 100.0;

  adjust_morale(winner, loser, 25);
  test::expect_true(winner.morale > 100.0,
                    "Winner morale should increase after victory");
  test::expect_true(loser.morale < 100.0,
                    "Loser morale should decrease after defeat");
}

void test_add_to_queue() {
  std::deque<std::string> q;

  // Empty string should be ignored
  add_to_queue(q, "");
  test::expect_true(q.empty(), "Empty string must not be queued");

  // Non-empty strings should be queued in FIFO order
  add_to_queue(q, "first line");
  test::expect_eq(q.size(), 1);
  test::expect_eq(q.front(), "first line");

  add_to_queue(q, "second line");
  test::expect_eq(q.size(), 2);
  test::expect_eq(q.back(), "second line");
}

void test_telegram_star() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Nonexistent star throws EntityNotFoundError (fail-fast)
  try {
    telegram_star(ctx.em, 999, 1, 0, "Unseen signal\n");
    test::expect_true(false,
                      "Expected EntityNotFoundError for nonexistent star");
  } catch (const EntityNotFoundError&) {
    // Expected fail-fast on missing internal star
  }

  // 2. Sol (Star 0) is inhabited by Player 1 and Player 2.
  // Player 1 Gov 0 sends telegram to Star 0:
  telegram_star(ctx.em, 0, 1, 0, "Welcome to Sol!\n");

  // Player 2 Gov 0 must receive the telegram
  auto p2_telegrams = ctx.db.telegram_get(2, 0);
  test::expect_eq(p2_telegrams.size(), 1);
  test::expect_contains(std::get<3>(p2_telegrams[0]), "Welcome to Sol!");

  // Player 1 Gov 0 is the sender, so they should not receive it
  auto p1_g0_telegrams = ctx.db.telegram_get(1, 0);
  test::expect_eq(p1_g0_telegrams.size(), 0);

  // 3. Sender with non-zero governor sends telegram:
  // Activate Gov 1 on Player 1
  ctx.em.mutate_race(1, [](Race& r) { r.governor[1].active = true; });

  telegram_star(ctx.em, 0, 1, 1, "Notice from Gov 1\n");

  // Player 1 Gov 0 should receive this notice (sender was Gov 1)
  p1_g0_telegrams = ctx.db.telegram_get(1, 0);
  test::expect_eq(p1_g0_telegrams.size(), 1);
  test::expect_contains(std::get<3>(p1_g0_telegrams[0]), "Notice from Gov 1");

  // Player 1 Gov 1 is the sender, so they should not receive it
  auto p1_g1_telegrams = ctx.db.telegram_get(1, 1);
  test::expect_eq(p1_g1_telegrams.size(), 0);

  // 4. Star not inhabited by Player 2
  ctx.em.mutate_star(2, [](Star& s) { s.clear_inhabited_by(player_t{2}); });
  telegram_star(ctx.em, 2, 1, 0, "Antares exclusive\n");

  // Player 2 should not receive any telegram for Star 2
  auto p2_new_telegrams = ctx.db.telegram_get(2, 0);
  // Size should still be 2 (from the two earlier Sol telegrams)
  test::expect_eq(p2_new_telegrams.size(), 2);
  for (const auto& t : p2_new_telegrams) {
    test::expect_false(std::get<3>(t).contains("Antares"));
  }
}

}  // namespace

int main() {
  test_adjust_morale();
  test_add_to_queue();
  test_telegram_star();

  std::println(std::cout, "✓ misc_test passed!");
  return 0;
}
