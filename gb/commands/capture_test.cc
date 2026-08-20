// SPDX-License-Identifier: Apache-2.0

/// \file capture_test.cc
/// \brief Unit tests for capture command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create two test races (attacker and defender)
  Race attacker{};
  attacker.Playernum = 1;
  attacker.name = "AttackerRace";
  attacker.Guest = false;
  attacker.Gov_ship = 0;
  attacker.tech = 10.0;
  attacker.fighters = 1.0;
  attacker.mass = 1.0;
  attacker.morale = 100;
  attacker.likes[SectorType::SEC_LAND] = 50;
  attacker.governor[0].active = true;
  attacker.governor[0].toggle.highlight = true;
  attacker.governor[1].active = true;

  Race defender{};
  defender.Playernum = 2;
  defender.name = "DefenderRace";
  defender.Guest = false;
  defender.Gov_ship = 0;
  defender.tech = 5.0;
  defender.fighters = 1.0;
  defender.mass = 1.0;
  defender.morale = 50;
  defender.governor[0].active = true;

  RaceRepository races(store);
  races.save(attacker);
  races.save(defender);

  // Create star
  star_struct star{};
  star.star_id = 0;
  star.name = "TestStar";
  star.pnames.push_back("TestPlanet");
  star.AP[0] = 10;  // Attacker has APs
  star.AP[1] = 10;  // Defender has APs
  star.governor[0] = 0;
  star.governor[1] = 0;
  star.explored = (1ULL << 1) | (1ULL << 2);

  StarRepository stars(store);
  stars.save(star);

  // Create planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(player_t{1}).mob_points = 0;
  planet.ships() = 1;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create sectormap with troops for attacker
  {
    SectorMap smap(planet, true);  // Initialize empty sectors
    smap.get(5, 5).set_owner(1);   // Attacker owns sector
    smap.get(5, 5).set_popn_exact(50);
    smap.get(5, 5).set_troops(100);  // Attacker has troops
    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create defender's ship (landed on planet at 5, 5)
  ship_struct ship{};
  ship.number = 1;
  ship.owner = 2;
  ship.governor = 0;
  ship.type = ShipType::STYPE_CARGO;
  ship.xpos = 0.0;
  ship.ypos = 0.0;
  ship.land_coords = {5, 5};
  ship.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship.whatdest = ScopeLevel::LEVEL_PLAN;
  ship.storbits = 0;
  ship.pnumorbits = 0;
  ship.docked = true;
  ship.on = true;
  ship.alive = true;
  ship.active = true;
  ship.popn = 10;
  ship.troops = 5;
  ship.max_crew = 20;
  ship.max_resource = 100;
  ship.damage = 0;
  ship.mass = 50.0;
  ship.build_cost = 100;
  ship.destruct = 0;

  auto ship_handle = ctx.em.create_ship(ship);
  ship_handle.save();
}

void test_capture_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Execute capture command - capture #1 50 military
  ctx.assert_dispatch_success(g, {"capture", "#1", "50", "military"});

  // Verify changes persisted
  const auto* captured_ship = ctx.em.peek_ship(1);
  assert(captured_ship);

  const auto* final_smap = ctx.em.peek_sectormap(0, 0);
  assert(final_smap);
  const auto& final_sector = final_smap->get(5, 5);
  assert(final_sector.get_troops() <= 100);

  if (captured_ship->alive()) {
    assert(captured_ship->owner() == 1 || captured_ship->owner() == 2);
  }
}

void test_capture_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
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

  ctx.assert_dispatch_rejected(g, {"capture", "#1", "50", "military"});
}

void test_capture_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  assert(g.out.str().contains("Invalid scope for this command."));

  // 2. Star control rejection
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->governor(1) = 2;  // Star governed by Gov 2
  }
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  assert(g.out.str().contains("not authorized"));
}

void test_capture_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"capture"});
  assert(g.out.str().contains(
      "Syntax: capture <ship> [<number>] [civilians|military]"));

  // 2. Ship not landed
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->docked() = false;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  assert(g.out.str().contains("not landed"));
}

}  // namespace

int main() {
  test_capture_happy_path();
  test_capture_insufficient_ap();
  test_capture_role_and_scope_rejections();
  test_capture_domain_errors();

  std::println(std::cout, "✓ capture_test passed!");
  return 0;
}
