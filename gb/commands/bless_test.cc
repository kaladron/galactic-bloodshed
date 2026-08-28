// SPDX-License-Identifier: Apache-2.0

/// \file bless_test.cc
/// \brief Unit tests for bless command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Test bless command - technology and money blessings
void test_bless_technology_and_money() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.tech = 10.0;
  race.mass = 1.0;
  race.metabolism = 1.0;
  race.governor[0].money = 100;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);

    star_struct star{};
    star.star_id = 1;
    star.name = "TestStar";
    star.pnames.push_back("TestPlanet");
    StarRepository star_repo(store);
    star_repo.save(star);

    Planet planet{};
    planet.star_id() = 1;
    planet.planet_order() = 0;
    PlanetRepository planet_repo(store);
    planet_repo.save(planet);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(true);

  // 1. Happy Path: Deity blesses technology
  ctx.assert_dispatch_success(g, {"bless", "1", "technology", "5"});
  test::expect_eq(ctx.em.peek_race(1)->tech, 15.0);

  // 2. Happy Path: Deity blesses money
  ctx.assert_dispatch_success(g, {"bless", "1", "money", "1000"});
  test::expect_eq(ctx.em.peek_race(1)->governor[0].money, 1100);
}

// Test bless command - permissions and scope restrictions
void test_bless_role_and_scope_rejection() {
  TestContext ctx;
  Race deity_race{};
  deity_race.Playernum = 1;
  deity_race.name = "DeityRace";
  deity_race.God = true;

  Race mortal_race{};
  mortal_race.Playernum = 2;
  mortal_race.name = "MortalRace";
  mortal_race.tech = 10.0;
  mortal_race.mass = 1.0;
  mortal_race.metabolism = 1.0;
  mortal_race.God = false;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(deity_race);
    races.save(mortal_race);

    star_struct star{};
    star.star_id = 1;
    StarRepository star_repo(store);
    star_repo.save(star);

    Planet planet{};
    planet.star_id() = 1;
    planet.planet_order() = 0;
    PlanetRepository planet_repo(store);
    planet_repo.save(planet);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Role Rejection: Mortal player (Player 2) is rejected
  ctx.setup_game_obj(g, 2, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(false);
  ctx.assert_dispatch_rejected(g, {"bless", "2", "technology", "5"});
  test::expect_eq(ctx.em.peek_race(2)->tech, 10.0);
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 2. Scope Rejection: Deity (Player 1) at LEVEL_UNIV scope is rejected
  g.out.str("");
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"bless", "2", "technology", "5"});
  test::expect_eq(ctx.em.peek_race(2)->tech, 10.0);
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
}

}  // namespace

int main() {
  test_bless_technology_and_money();
  test_bless_role_and_scope_rejection();

  std::println(std::cout, "✓ bless_test passed!");
  return 0;
}
