// SPDX-License-Identifier: Apache-2.0

/// \file misc_test.cc
/// \brief Unit tests for miscellaneous entity and turn helpers (adjust_morale,
/// add_to_queue, telegram_star).

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
  const auto races = ctx.with_races();

  // 1. Nonexistent star throws EntityNotFoundError (fail-fast)
  try {
    telegram_star(ctx.em, 999, races.empire, 1, "Unseen signal\n");
    test::expect_true(false,
                      "Expected EntityNotFoundError for nonexistent star");
  } catch (const EntityNotFoundError&) {
    // Expected fail-fast on missing internal star
  }

  // 2. Sol (Star 1) is inhabited by empire and nomad in with_races().
  // Empire Gov 1 (leader) sends telegram to Star 1:
  telegram_star(ctx.em, 1, races.empire, 1, "Welcome to Sol!\n");

  // Nomad Gov 1 (leader) must receive the telegram
  auto nomad_telegrams = ctx.em.get_telegrams(races.nomad, 1);
  test::expect_eq(nomad_telegrams.size(), 1);
  test::expect_contains(nomad_telegrams[0].message, "Welcome to Sol!");

  // Empire Gov 1 is the sender, so they should not receive it
  auto emp_g1_telegrams = ctx.em.get_telegrams(races.empire, 1);
  test::expect_eq(emp_g1_telegrams.size(), 0);

  // 3. Sender with subordinate governor sends telegram:
  // Activate Gov 2 on empire
  ctx.em.mutate_race(races.empire, [](Race& r) { r.appoint_governor(2); });

  telegram_star(ctx.em, 1, races.empire, 2, "Notice from Gov 2\n");

  // Empire Gov 1 (leader) should receive this notice (sender was Gov 2)
  emp_g1_telegrams = ctx.em.get_telegrams(races.empire, 1);
  test::expect_eq(emp_g1_telegrams.size(), 1);
  test::expect_contains(emp_g1_telegrams[0].message, "Notice from Gov 2");

  // Empire Gov 2 is the sender, so they should not receive it
  auto emp_g2_telegrams = ctx.em.get_telegrams(races.empire, 2);
  test::expect_eq(emp_g2_telegrams.size(), 0);

  // 4. Star 3 (Antares) is inhabited by newcomer, not nomad
  telegram_star(ctx.em, 3, races.newcomer, 1, "Antares exclusive\n");

  // Nomad should not receive any telegram for Star 3
  auto nomad_new_telegrams = ctx.em.get_telegrams(races.nomad, 1);
  // Size should still be 2 (from the two earlier Sol telegrams)
  test::expect_eq(nomad_new_telegrams.size(), 2);
  for (const auto& t : nomad_new_telegrams) {
    test::expect_false(t.message.contains("Antares"));
  }
}

}  // namespace

int main() {
  test_adjust_morale();
  test_telegram_star();

  std::println(std::cout, "✓ misc_test passed!");
  return 0;
}
