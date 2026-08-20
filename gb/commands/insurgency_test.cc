// SPDX-License-Identifier: Apache-2.0

/// \file insurgency_test.cc
/// \brief Unit tests for insurgency command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race (instigator)
  Race race{};
  race.Playernum = 1;
  race.name = "Rebels";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].money = 10000;
  race.governor[0].toggle.highlight = true;
  race.governor[1].active = true;
  race.morale = 100;
  race.fighters = 10;

  RaceRepository races(store);
  races.save(race);

  // Create target race
  Race target{};
  target.Playernum = 2;
  target.name = "Oppressors";
  target.Guest = false;
  target.governor[0].active = true;
  target.morale = 50;
  target.fighters = 5;
  races.save(target);

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
  planet.info(player_t{1}).popn = 100;
  planet.info(player_t{1}).troops = 50;
  planet.info(player_t{2}).popn = 1000;
  planet.info(player_t{2}).troops = 100;
  planet.info(player_t{2}).numsectsowned = 5;
  planet.info(player_t{2}).tax = 10;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    for (int i = 0; i < 5; i++) {
      smap.get(i, 0).set_owner(2);
      smap.get(i, 0).set_popn_exact(200);
      smap.get(i, 0).set_condition(SectorType::SEC_MOUNT);
    }

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }
}

void test_insurgency_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"insurgency", "2", "5000"}, 10);

  // Verify race money decreased
  ctx.em.clear_cache();
  const auto* saved_race = ctx.em.peek_race(1);
  assert(saved_race);
  assert(saved_race->governor[0].money == 5000);
}

void test_insurgency_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 5 (< 10 required)
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->AP(1) = 5;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  assert(g.out.str().contains("action points"));

  // Money must not have been deducted
  const auto* race = ctx.em.peek_race(1);
  assert(race->governor[0].money == 10000);
}

void test_insurgency_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
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
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  assert(g.out.str().contains("not authorized"));
}

void test_insurgency_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"insurgency", "2"});
  assert(g.out.str().contains("Syntax: insurgency <race> <money>"));

  // 2. Revolt against self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "1", "5000"});
  assert(g.out.str().contains("yourself"));

  // 3. Not enough money
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "50000"});
  assert(g.out.str().contains("Nice try"));
}

}  // namespace

int main() {
  test_insurgency_happy_path();
  test_insurgency_insufficient_ap();
  test_insurgency_role_and_scope_rejections();
  test_insurgency_domain_errors();

  std::println(std::cout, "✓ insurgency_test passed!");
  return 0;
}
