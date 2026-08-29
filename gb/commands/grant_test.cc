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
  JsonStore store(ctx.db);

  // Create test race with multiple governors
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.governor[0].money = 1000;
  race.governor[1].active = true;
  race.governor[1].money = 500;
  race.governor[1].name = "SubGov";

  RaceRepository races(store);
  races.save(race);

  // Create a star system
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[player_t{1}] = 0;
  star_data.name = "SectorStar";
  Star star{star_data};
  StarRepository stars(store);
  stars.save(star);

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

  // 3. Grant star when scoped to star
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"grant", "1", "star"});
  const auto* saved_star = ctx.em.peek_star(1);
  test::expect_ne(saved_star, nullptr);
  test::expect_eq(saved_star->governor(player_t{1}), 1);
  std::println(std::cout, "    ✓ Star granted to governor");

  // 4. Role check: Governor cannot grant
  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "1", "money", "100"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  // 5. Inactive governor rejection
  g.set_governor(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "5", "money", "100"});
  test::expect_contains(g.out.str(), "That governor is not active.");
  std::println(std::cout, "    ✓ Inactive governor rejection verified");

  // 6. Bad governor number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"grant", "99", "money", "100"});
  test::expect_contains(g.out.str(), "Bad governor number.");
  std::println(std::cout, "    ✓ Bad governor number rejection verified");
}

}  // namespace

int main() {
  test_grant_dispatch();
  std::println(std::cout, "\n✅ All grant tests passed!");
  return 0;
}
