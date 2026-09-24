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

  winner.adjust_morale(loser, 25);
  test::expect_true(winner.morale > 100.0,
                    "Winner morale should increase after victory");
  test::expect_true(loser.morale < 100.0,
                    "Loser morale should decrease after defeat");
}

void test_telegram_star() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Nonexistent star throws EntityNotFoundError (fail-fast)
  try {
    telegram_star(ctx.em, 999, 1, 1, "Unseen signal\n");
    test::expect_true(false,
                      "Expected EntityNotFoundError for nonexistent star");
  } catch (const EntityNotFoundError&) {
    // Expected fail-fast on missing internal star
  }

  // 2. Sol (Star 1) is inhabited by Player 1 and Player 2.
  // Player 1 Gov 1 (leader) sends telegram to Star 1:
  telegram_star(ctx.em, 1, 1, 1, "Welcome to Sol!\n");

  // Player 2 Gov 1 (leader) must receive the telegram
  auto p2_telegrams = ctx.db.telegram_get(2, 1);
  test::expect_eq(p2_telegrams.size(), 1);
  test::expect_contains(std::get<3>(p2_telegrams[0]), "Welcome to Sol!");

  // Player 1 Gov 1 is the sender, so they should not receive it
  auto p1_g1_telegrams = ctx.db.telegram_get(1, 1);
  test::expect_eq(p1_g1_telegrams.size(), 0);

  // 3. Sender with subordinate governor sends telegram:
  // Activate Gov 2 on Player 1
  ctx.em.mutate_race(1, [](Race& r) { r.appoint_governor(2); });

  telegram_star(ctx.em, 1, 1, 2, "Notice from Gov 2\n");

  // Player 1 Gov 1 (leader) should receive this notice (sender was Gov 2)
  p1_g1_telegrams = ctx.db.telegram_get(1, 1);
  test::expect_eq(p1_g1_telegrams.size(), 1);
  test::expect_contains(std::get<3>(p1_g1_telegrams[0]), "Notice from Gov 2");

  // Player 1 Gov 2 is the sender, so they should not receive it
  auto p1_g2_telegrams = ctx.db.telegram_get(1, 2);
  test::expect_eq(p1_g2_telegrams.size(), 0);

  // 4. Star not inhabited by Player 2
  ctx.em.mutate_star(3, [](Star& s) { s.clear_inhabited_by(player_t{2}); });
  telegram_star(ctx.em, 3, 1, 1, "Antares exclusive\n");

  // Player 2 should not receive any telegram for Star 3
  auto p2_new_telegrams = ctx.db.telegram_get(2, 1);
  // Size should still be 2 (from the two earlier Sol telegrams)
  test::expect_eq(p2_new_telegrams.size(), 2);
  for (const auto& t : p2_new_telegrams) {
    test::expect_false(std::get<3>(t).contains("Antares"));
  }
}

}  // namespace

int main() {
  test_adjust_morale();
  test_telegram_star();

  std::println(std::cout, "✓ misc_test passed!");
  return 0;
}
