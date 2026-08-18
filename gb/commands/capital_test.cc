// SPDX-License-Identifier: Apache-2.0

/// \file capital_test.cc
/// \brief Unit tests for capital command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

// Test designating capital ship successfully
void test_designate_capital_success() {
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars(store);
  ShipRepository ships(store);

  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Gov_ship = 0;
  races.save(race);

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.AP[0] = 100;
  Star s_entity{star};
  stars.save(s_entity);

  Ship ship{};
  ship.number() = 1;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.storbits() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ship.xpos() = 10.0;
  ship.ypos() = 10.0;
  ship.alive() = true;
  ship.active() = true;
  ship.docked() = true;
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_snum(1);

  // 1. Happy Path: Leader designates capital (deducts 50 AP)
  ctx.assert_dispatch_success(g, {"capital", "1"},
                              /*expected_star_ap_deducted=*/50);
  assert(ctx.em.peek_race(1)->Gov_ship == 1);
  assert(ctx.em.peek_star(1)->AP(1) == 50);

  // 2. Query mode: Free inquiry (0 AP)
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  assert(ctx.em.peek_star(1)->AP(1) == 50);
}

// Test capital designation rejection due to insufficient AP
void test_capital_insufficient_ap() {
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars(store);
  ShipRepository ships(store);

  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Gov_ship = 0;
  races.save(race);

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.AP[0] = 20;  // Need 50
  Star s_entity{star};
  stars.save(s_entity);

  Ship ship{};
  ship.number() = 1;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.storbits() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ship.alive() = true;
  ship.active() = true;
  ship.docked() = true;
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_snum(1);

  // Insufficient AP: Rejected with 0 AP deducted
  ctx.assert_dispatch_rejected(g, {"capital", "1"});
  assert(ctx.em.peek_race(1)->Gov_ship == 0);
  assert(ctx.em.peek_star(1)->AP(1) == 20);
}

// Test capital designation permissions and landed ship checks
void test_capital_role_and_domain_errors() {
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars(store);
  ShipRepository ships(store);

  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Gov_ship = 0;
  races.save(race);

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.AP[0] = 100;
  Star s_entity{star};
  stars.save(s_entity);

  // Ship not landed (orbiting star)
  Ship ship{};
  ship.number() = 1;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.storbits() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.whatdest() = ScopeLevel::LEVEL_STAR;
  ship.alive() = true;
  ship.active() = true;
  ship.docked() = false;
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Role Rejection: Governor 1 cannot designate capital
  ctx.setup_game_obj(g, 1, 1);
  g.set_snum(1);
  ctx.assert_dispatch_rejected(g, {"capital", "1"});
  assert(ctx.em.peek_race(1)->Gov_ship == 0);
  assert(g.out.str().contains(
      "Only the leader (Governor 0) may use this command."));

  // 2. Domain Error: Ship is not landed
  g.out.str("");
  ctx.setup_game_obj(g, 1, 0);
  g.set_snum(1);
  ctx.assert_dispatch_rejected(g, {"capital", "1"});
  assert(ctx.em.peek_race(1)->Gov_ship == 0);
  assert(g.out.str().contains("Try landing this ship first!"));
}

}  // namespace

int main() {
  test_designate_capital_success();
  test_capital_insufficient_ap();
  test_capital_role_and_domain_errors();

  std::println(std::cout, "✓ capital_test passed!");
  return 0;
}
