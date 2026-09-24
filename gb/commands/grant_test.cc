// SPDX-License-Identifier: Apache-2.0

/// \file grant_test.cc
/// \brief Test grant command functionality, governor transfers, and validation
/// rules.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_grant_dispatch() {
  std::println(std::cout, "Test: grant command dispatch and governor grants");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().money = 1000;
    r.appoint_governor(2, {.name = "SubGov", .money = 500});
  });

  shipnum_t cruiser_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                             .owned_by(1, 1)
                             .in_star_orbit(1)
                             .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Grant money to governor
  ctx.assert_dispatch_success(g, {"grant", "2", "money", "200"});
  const auto* saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->leader().money, 800);
  test::expect_eq(saved_race->governor(2).money, 700);
  std::println(std::cout, "    ✓ Money granted to governor");

  // 2. Dock money from governor
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "2", "money", "-100"});
  saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->leader().money, 900);
  test::expect_eq(saved_race->governor(2).money, 600);
  std::println(std::cout, "    ✓ Money docked from governor");

  // 3. Grant money clamped to leader treasury
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "2", "money", "999999"});
  saved_race = ctx.em.peek_race(1);
  test::expect_eq(saved_race->leader().money, 0);
  test::expect_eq(saved_race->governor(2).money, 1500);
  std::println(std::cout, "    ✓ Positive money clamped to treasury");

  // 4. Dock money clamped to governor treasury
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "2", "money", "-999999"});
  saved_race = ctx.em.peek_race(1);
  test::expect_eq(saved_race->leader().money, 1500);
  test::expect_eq(saved_race->governor(2).money, 0);
  std::println(std::cout, "    ✓ Negative money clamped to treasury");

  // 5. Money missing amount or bad number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "2", "money"});
  test::expect_contains(g.out.str(), "Indicate the amount of money.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "2", "money", "not_a_number"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  // 6. Grant star when scoped to star
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "2", "star"});
  const auto* saved_star = ctx.em.peek_star(1);
  test::expect_ne(saved_star, nullptr);
  test::expect_eq(saved_star->governor(player_t{1}), 2);
  std::println(std::cout, "    ✓ Star granted to governor");

  // 7. Grant star when not scoped to star
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "2", "star"});
  test::expect_contains(g.out.str(), "Please cs to the star system first.");

  // 8. Grant ship to governor
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"grant", "2", "ship", std::format("#{}", cruiser_id.value)});
  const auto* saved_ship = ctx.em.peek_ship(cruiser_id);
  test::expect_ne(saved_ship, nullptr);
  test::expect_eq(saved_ship->governor(), 2);
  test::expect_contains(g.out.str(), "granted to \"SubGov\"");

  // 9. Grant ship missing shiplist argument
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "2", "ship"});
  test::expect_contains(g.out.str(),
                        "Syntax: grant <governor> ship <shiplist>");

  // 10. Grant unknown target
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "2", "lasers"});
  test::expect_contains(g.out.str(), "You can't grant that.");

  // 11. Inactive governor rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "5", "money", "100"});
  test::expect_contains(g.out.str(), "That governor is not active.");
  std::println(std::cout, "    ✓ Inactive governor rejection verified");

  // 12. Bad governor number (< 1)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "0", "money", "100"});
  test::expect_contains(g.out.str(), "Bad governor number.");
  std::println(std::cout, "    ✓ Bad governor number rejection verified");

  // 13. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "grant")
      .with_valid_argv({"grant", "2", "money", "100"})
      .with_invalid_argv({"grant", "0", "money", "100"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_grant_dispatch();
  std::println(std::cout, "\n✅ All grant tests passed!");
  return 0;
}
