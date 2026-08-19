// SPDX-License-Identifier: Apache-2.0

/// \file fire_test.cc
/// \brief Unit tests for fire and cew commands

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  // Create test race (Attacker)
  Race race{};
  race.Playernum = 1;
  race.name = "Attacker";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;
  race.tech = 100.0;
  race.morale = 100;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create target race (Defender)
  Race target_race{};
  target_race.Playernum = 2;
  target_race.name = "Defender";
  target_race.Guest = false;
  target_race.governor[0].active = true;
  target_race.tech = 100.0;
  target_race.morale = 100;
  races.save(target_race);

  // Create star system
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "CombatStar";
  ss.pnames.emplace_back("CombatPlanet");
  ss.AP[0] = 100;  // Player 1 APs
  ss.AP[1] = 100;  // Player 2 APs
  ss.explored = (1ULL << 1) | (1ULL << 2);
  StarRepository stars(store);
  stars.save(ss);

  // Create attacker ship - armed with guns
  ship_struct attacker{};
  attacker.number = 1;
  attacker.owner = 1;
  attacker.governor = 0;
  attacker.alive = true;
  attacker.active = true;
  attacker.type = ShipType::STYPE_BATTLE;
  attacker.guns = PRIMARY;
  attacker.primary = 10;
  attacker.primtype = GTYPE_LIGHT;
  attacker.popn = 10;
  attacker.troops = 10;
  attacker.retaliate = 100;
  attacker.destruct = 100;
  attacker.fuel = 1000.0;
  attacker.size = 100;
  attacker.tech = 100.0;
  attacker.whatorbits = ScopeLevel::LEVEL_STAR;
  attacker.storbits = 0;
  attacker.xpos = 100.0;
  attacker.ypos = 200.0;
  attacker.mass = 100.0;
  attacker.build_cost = 100;

  auto attacker_handle = ctx.em.create_ship(attacker);
  attacker_handle.save();

  // Create target ship
  ship_struct target{};
  target.number = 2;
  target.owner = 2;
  target.governor = 0;
  target.alive = true;
  target.active = true;
  target.type = ShipType::STYPE_CARGO;
  target.whatorbits = ScopeLevel::LEVEL_STAR;
  target.storbits = 0;
  target.xpos = 110.0;
  target.ypos = 210.0;
  target.armor = 10;
  target.damage = 0;
  target.size = 50;
  target.popn = 10;
  target.tech = 100.0;
  target.mass = 50.0;
  target.build_cost = 50;

  auto target_handle = ctx.em.create_ship(target);
  target_handle.save();

  // Create CEW equipped ship
  ship_struct cew_ship{};
  cew_ship.number = 3;
  cew_ship.owner = 1;
  cew_ship.governor = 0;
  cew_ship.alive = true;
  cew_ship.active = true;
  cew_ship.on = true;
  cew_ship.type = ShipType::STYPE_BATTLE;
  cew_ship.guns = PRIMARY;
  cew_ship.primary = 10;
  cew_ship.primtype = GTYPE_LIGHT;
  cew_ship.cew = 20;
  cew_ship.cew_range = 1000;
  cew_ship.mounted = 1;
  cew_ship.fuel = 1000.0;
  cew_ship.size = 100;
  cew_ship.popn = 10;
  cew_ship.tech = 100.0;
  cew_ship.whatorbits = ScopeLevel::LEVEL_STAR;
  cew_ship.storbits = 0;
  cew_ship.xpos = 100.0;
  cew_ship.ypos = 200.0;
  cew_ship.mass = 100.0;
  cew_ship.build_cost = 100;

  auto cew_handle = ctx.em.create_ship(cew_ship);
  cew_handle.save();
}

void test_fire_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Execute fire command: Ship #1 attacks Ship #2 with strength 10
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "10"}, 1);

  // Verify ships still exist in database (persisted via EntityManager)
  const auto* ship1 = ctx.em.peek_ship(1);
  assert(ship1);
  assert(ship1->number() == 1);
  assert(ship1->destruct() < 100);

  const auto* ship2 = ctx.em.peek_ship(2);
  assert(ship2);
  assert(ship2->number() == 2);

  // 2. Execute cew command: Ship #3 attacks Ship #2 with CEWs
  g.out.str("");
  ctx.assert_dispatch_success(g, {"cew", "#3", "#2"}, 1);
  const auto* ship3 = ctx.em.peek_ship(3);
  assert(ship3);
  assert(ship3->fuel() < 1000.0);
}

void test_fire_universe_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Move attacker ship to Universe scope and target at Star scope
  {
    auto s1 = ctx.em.get_ship(1);
    s1->whatorbits() = ScopeLevel::LEVEL_UNIV;
    auto s2 = ctx.em.get_ship(2);
    s2->whatorbits() = ScopeLevel::LEVEL_STAR;
  }

  // Set universe AP
  {
    auto u = ctx.em.get_universe();
    u->AP[0] = 50;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  bool ok = ctx.dispatch(g, {"fire", "#1", "#2", "10"});
  assert(ok);
  assert(ctx.em.peek_universe()->AP[0] == 49);
}

void test_fire_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  assert(g.out.str().contains("action points"));
}

void test_fire_role_and_guest_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = 3;
  guest_race.name = "GuestAttacker";
  guest_race.Guest = true;
  guest_race.governor[0].active = true;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Guest race rejection for fire
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  assert(g.out.str().contains("Guest races cannot use this command."));

  // 2. Guest race rejection for cew
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#3", "#2"});
  assert(g.out.str().contains("Guest races cannot use this command."));
}

void test_fire_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"fire", "#1"});
  assert(g.out.str().contains("Syntax: fire <ship> <target> [<strength>]"));

  // 2. Target self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#1", "10"});
  assert(g.out.str().contains("Get real."));

  // 3. Inactive ship
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->active() = false;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  assert(g.out.str().contains("inactive"));
}

}  // namespace

int main() {
  test_fire_happy_paths();
  test_fire_universe_ap();
  test_fire_insufficient_ap();
  test_fire_role_and_guest_rejections();
  test_fire_domain_errors();

  std::println(std::cout, "✓ fire_test passed!");
  return 0;
}
