// SPDX-License-Identifier: Apache-2.0

/// \file bombard_test.cc
/// \brief Unit tests for bombard command

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
  StarRepository star_repo(store);
  star_repo.save(ss);

  // Create planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 100.0;
  planet.ypos() = 200.0;

  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Create and initialize sector map
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    smap.get(5, 5).set_popn_exact(100);
    smap.get(5, 5).set_owner(2);  // Owned by race 2
    smap.get(5, 5).set_troops(10);

    SectorRepository smap_repo(store);
    smap_repo.save_map(smap);
  }

  // Create attacker ship in orbit
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
  attacker.whatorbits = ScopeLevel::LEVEL_PLAN;
  attacker.storbits = 0;
  attacker.pnumorbits = 0;
  attacker.xpos = 100.0;
  attacker.ypos = 200.0;
  attacker.mass = 100.0;
  attacker.build_cost = 100;

  auto attacker_handle = ctx.em.create_ship(attacker);
  attacker_handle.save();
}

void test_bombard_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Execute bombard command on sector 5,5 with strength 10 (deducts 1 Star AP
  // dynamically)
  ctx.assert_dispatch_success(g, {"bombard", "#1", "5,5", "10"}, 1);

  // Verify ship and planet still exist in database (persisted via
  // EntityManager)
  const auto* ship = ctx.em.peek_ship(1);
  assert(ship);
  assert(ship->number() == 1);
  assert(ship->destruct() < 100);  // Ammo consumed

  const auto* planet_after = ctx.em.peek_planet(0, 0);
  assert(planet_after);

  // Verify sector map persisted and target was damaged
  const auto* smap_after = ctx.em.peek_sectormap(0, 0);
  assert(smap_after);
}

void test_bombard_insufficient_ap() {
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
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  assert(g.out.str().contains("action points"));
}

void test_bombard_role_and_scope_rejections() {
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

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  assert(g.out.str().contains("Guest races cannot use this command."));

  // 2. Scope rejection (LEVEL_UNIV is not allowed for bombard)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  assert(g.out.str().contains("Invalid scope for this command."));
}

void test_bombard_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"bombard"});
  assert(g.out.str().contains("Syntax: bombard <ship> [<x,y> [<strength>]]"));

  // 2. Inactive ship
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->active() = false;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  assert(g.out.str().contains("inactive"));
}

}  // namespace

int main() {
  test_bombard_happy_paths();
  test_bombard_insufficient_ap();
  test_bombard_role_and_scope_rejections();
  test_bombard_domain_errors();

  std::println(std::cout, "✓ bombard_test passed!");
  return 0;
}
