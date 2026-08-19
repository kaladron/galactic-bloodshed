// SPDX-License-Identifier: Apache-2.0

/// \file move_popn_test.cc
/// \brief Unit tests for move and deploy commands

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
  race.fighters = 10;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.pnames.emplace_back("Test Planet");
  star.AP[0] = 100;
  star.governor[0] = 0;
  star.governor[1] = 0;
  star.explored = (1ULL << 1);

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet, true);

    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn_exact(1000);
    smap.get(5, 5).set_troops(500);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);

    smap.get(5, 6).set_owner(1);
    smap.get(5, 6).set_popn_exact(0);
    smap.get(5, 6).set_troops(0);
    smap.get(5, 6).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }
}

void test_move_popn_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Test move command - 'k' moves south (y+1)
  ctx.assert_dispatch_success(g, {"move", "5,5", "k", "500"});

  // Verify population moved
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(0, 0);
  assert(saved_smap);

  const auto& source_sect = saved_smap->get(5, 5);
  assert(source_sect.get_popn() == 500);

  const auto& dest_sect = saved_smap->get(5, 6);
  assert(dest_sect.get_popn() == 500);

  // 2. Test deploy command - deploy 200 troops
  g.out.str("");
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});

  ctx.em.clear_cache();
  const auto* smap2 = ctx.em.peek_sectormap(0, 0);
  assert(smap2);
  assert(smap2->get(5, 5).get_troops() == 300);
  assert(smap2->get(5, 6).get_troops() == 200);
}

void test_move_popn_insufficient_ap() {
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

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  assert(g.out.str().contains("action points"));
}

void test_move_popn_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV is not allowed for move/deploy)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  assert(g.out.str().contains("Invalid scope for this command."));

  // 2. Star control rejection
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->governor(1) = 2;  // Player 1, Star governed by Gov 2
  }
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  assert(g.out.str().contains("not authorized"));
}

void test_move_popn_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"move", "5,5"});
  assert(g.out.str().contains("Syntax: move <from_sector> <path> [<amount>]"));

  // 2. Origin coordinates illegal
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "99,99", "k"});
  assert(g.out.str().contains("illegal"));

  // 3. Bad value - more people than available in sector
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "99999"});
  assert(g.out.str().contains("Bad value"));
}

}  // namespace

int main() {
  test_move_popn_happy_paths();
  test_move_popn_insufficient_ap();
  test_move_popn_role_and_scope_rejections();
  test_move_popn_domain_errors();

  std::println(std::cout, "✓ move_popn_test passed!");
  return 0;
}
