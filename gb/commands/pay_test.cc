// SPDX-License-Identifier: Apache-2.0

/// \file pay_test.cc
/// \brief Test pay command functionality, treasury transfers, and role
/// validation.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_pay_dispatch() {
  std::println(std::cout, "Test: pay command dispatch and treasury transfer");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create payer race via repository
  Race payer{};
  payer.Playernum = 1;
  payer.name = "Payer";
  payer.Guest = false;
  payer.governor[0].money = 10000;
  payer.governor[0].active = true;

  // Create payee race via repository
  Race payee{};
  payee.Playernum = 2;
  payee.name = "Payee";
  payee.Guest = false;
  payee.governor[0].money = 1000;
  payee.governor[0].active = true;

  RaceRepository races(store);
  races.save(payer);
  races.save(payee);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Pay 500 from player 1 to player 2
  ctx.assert_dispatch_success(g, {"pay", "2", "500"});
  const auto* saved_payer = ctx.em.peek_race(1);
  const auto* saved_payee = ctx.em.peek_race(2);
  test::expect_ne(saved_payer, nullptr);
  test::expect_ne(saved_payee, nullptr);
  test::expect_eq(saved_payer->governor[0].money, 9500);
  test::expect_eq(saved_payee->governor[0].money, 1500);
  std::println(std::cout, "    ✓ Money transfer saved correctly");

  // 2. Role check: Governor != 0 cannot pay
  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pay", "2", "500"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  // 3. Insufficient funds rejection
  g.set_governor(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pay", "2", "999999"});
  test::expect_contains(g.out.str(), "You don't have that much money to give!");
  std::println(std::cout, "    ✓ Insufficient funds rejection verified");

  // 4. Negative amount rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pay", "2", "-100"});
  test::expect_contains(
      g.out.str(), "You have to give a player a positive amount of money.");
  std::println(std::cout, "    ✓ Negative amount rejection verified");

  // 5. Invalid player rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pay", "99", "100"});
  test::expect_true(g.out.str().contains("No such player.") ||
                    g.out.str().contains("Alien race not found."));
  std::println(std::cout, "    ✓ Invalid player rejection verified");

  // 6. Cumulative transfer check
  g.out.str("");
  ctx.assert_dispatch_success(g, {"pay", "2", "1000"});
  saved_payer = ctx.em.peek_race(1);
  saved_payee = ctx.em.peek_race(2);
  test::expect_ne(saved_payer, nullptr);
  test::expect_ne(saved_payee, nullptr);
  test::expect_eq(saved_payer->governor[0].money, 8500);
  test::expect_eq(saved_payee->governor[0].money, 2500);
  std::println(std::cout,
               "    ✓ Cumulative transfer verified (payer: 8500, payee: 2500)");
}

}  // namespace

int main() {
  test_pay_dispatch();
  std::println(std::cout, "\n✅ All pay command tests passed!");
  return 0;
}
