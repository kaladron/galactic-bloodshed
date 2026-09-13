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
    r.governor[0].money = 1000;
    r.governor[1].active = true;
    r.governor[1].money = 500;
    r.governor[1].name = "SubGov";
  });

  shipnum_t cruiser_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                             .owned_by(1, 0)
                             .in_star_orbit(0)
                             .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Grant money to governor
  ctx.assert_dispatch_success(g, {"grant", "1", "money", "200"});
  const auto* saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->governor[0].money, 800);
  test::expect_eq(saved_race->governor[1].money, 700);
  std::println(std::cout, "    ✓ Money granted to governor");

  // 2. Dock money from governor
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "1", "money", "-100"});
  saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->governor[0].money, 900);
  test::expect_eq(saved_race->governor[1].money, 600);
  std::println(std::cout, "    ✓ Money docked from governor");

  // 3. Grant money clamped to leader treasury
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "1", "money", "999999"});
  saved_race = ctx.em.peek_race(1);
  test::expect_eq(saved_race->governor[0].money, 0);
  test::expect_eq(saved_race->governor[1].money, 1500);
  std::println(std::cout, "    ✓ Positive money clamped to treasury");

  // 4. Dock money clamped to governor treasury
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "1", "money", "-999999"});
  saved_race = ctx.em.peek_race(1);
  test::expect_eq(saved_race->governor[0].money, 1500);
  test::expect_eq(saved_race->governor[1].money, 0);
  std::println(std::cout, "    ✓ Negative money clamped to treasury");

  // 5. Money missing amount or bad number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "money"});
  test::expect_contains(g.out.str(), "Indicate the amount of money.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "money", "not_a_number"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  // 6. Grant star when scoped to star
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "1", "star"});
  const auto* saved_star = ctx.em.peek_star(0);
  test::expect_ne(saved_star, nullptr);
  test::expect_eq(saved_star->governor(player_t{1}), 1);
  std::println(std::cout, "    ✓ Star granted to governor");

  // 7. Grant star when not scoped to star
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "star"});
  test::expect_contains(g.out.str(), "Please cs to the star system first.");

  // 8. Grant ship to governor
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"grant", "1", "ship", std::format("#{}", cruiser_id.value)});
  const auto* saved_ship = ctx.em.peek_ship(cruiser_id);
  test::expect_ne(saved_ship, nullptr);
  test::expect_eq(saved_ship->governor(), 1);
  test::expect_contains(g.out.str(), "granted to \"SubGov\"");

  // 9. Grant ship missing shiplist argument
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "ship"});
  test::expect_contains(g.out.str(),
                        "Syntax: grant <governor> ship <shiplist>");

  // 10. Grant unknown target
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "lasers"});
  test::expect_contains(g.out.str(), "You can't grant that.");

  // 11. Inactive governor rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "5", "money", "100"});
  test::expect_contains(g.out.str(), "That governor is not active.");
  std::println(std::cout, "    ✓ Inactive governor rejection verified");

  // 12. Bad governor number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "99", "money", "100"});
  test::expect_contains(g.out.str(), "Bad governor number.");
  std::println(std::cout, "    ✓ Bad governor number rejection verified");

  // 13. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "grant")
      .with_valid_argv({"grant", "1", "money", "100"})
      .with_invalid_argv({"grant", "99", "money", "100"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_grant_dispatch();
  std::println(std::cout, "\n✅ All grant tests passed!");
  return 0;
}
