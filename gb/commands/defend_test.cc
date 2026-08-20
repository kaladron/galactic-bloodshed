// SPDX-License-Identifier: Apache-2.0

/// \file defend_test.cc
/// \brief Unit tests for defend command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Testers";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;
  race.governor[1].active = true;

  RaceRepository races(store);
  races.save(race);

  // Create enemy race
  Race enemy{};
  enemy.Playernum = 2;
  enemy.name = "Enemies";
  enemy.Guest = false;
  enemy.governor[0].active = true;
  races.save(enemy);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.AP[0] = 100;
  star.pnames.emplace_back("Test Planet");
  star.governor[0] = 0;
  star.governor[1] = 0;
  star.explored = (1ULL << 1) | (1ULL << 2);

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(player_t{1}).numsectsowned = 1;
  planet.info(player_t{1}).guns = 50;
  planet.info(player_t{1}).destruct = 100;
  planet.xpos() = 0.0;
  planet.ypos() = 0.0;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create attacking ship
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 2;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::OTYPE_FACTORY;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.xpos() = 0.0;
  ship.ypos() = 0.0;
  ship.armor() = 100;
  ship.size() = Shipdata[ShipType::OTYPE_FACTORY][ABIL_BUILD];
  ship.tech() = 100.0;
  ship.build_cost() = 100;

  ShipRepository ships(store);
  ships.save(ship);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn_exact(1000);
    smap.get(5, 5).set_troops(500);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }
}

void test_defend_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"defend", "1", "5,5", "25"});

  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  assert(saved_planet);
  assert(saved_planet->info(player_t{1}).destruct < 100);
}

void test_defend_insufficient_ap() {
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

  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  assert(g.out.str().contains("action points"));
}

void test_defend_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
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
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  assert(g.out.str().contains("not authorized"));
}

void test_defend_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args (< 3 args)
  ctx.assert_dispatch_rejected(g, {"defend", "1"});
  assert(g.out.str().contains("Syntax: defend <ship> <sector> [<strength>]"));

  // 2. Bad ship number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "abc", "5,5"});
  assert(g.out.str().contains("Bad ship number"));

  // 3. Bad sector format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "bad_coords"});
  assert(g.out.str().contains("Bad format"));
}

}  // namespace

int main() {
  test_defend_happy_path();
  test_defend_insufficient_ap();
  test_defend_role_and_scope_rejections();
  test_defend_domain_errors();

  std::println(std::cout, "✓ defend_test passed!");
  return 0;
}
