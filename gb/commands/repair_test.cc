// SPDX-License-Identifier: Apache-2.0

/// \file repair_test.cc
/// \brief Unit tests for repair command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "Testers";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);

  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";

  StarRepository stars(store);
  stars.save(star);

  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).resource = 1000;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap with wasted sectors
  {
    SectorMap smap(planet, true);

    smap.get(3, 3).set_owner(1);
    smap.get(3, 3).set_condition(SectorType::SEC_WASTED);
    smap.get(3, 3).set_type(SectorType::SEC_MOUNT);
    smap.get(3, 3).set_fert(50);

    smap.get(4, 4).set_owner(1);
    smap.get(4, 4).set_condition(SectorType::SEC_WASTED);
    smap.get(4, 4).set_type(SectorType::SEC_LAND);
    smap.get(4, 4).set_fert(30);

    smap.get(5, 5).set_owner(0);
    smap.get(5, 5).set_condition(SectorType::SEC_WASTED);
    smap.get(5, 5).set_type(SectorType::SEC_SEA);
    smap.get(5, 5).set_fert(20);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }
}

void test_repair_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"repair", "3:5,3:5"});
  assert(g.out.str().contains("3 sectors repaired at a cost of"));

  // Verify sectors were repaired
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(0, 0);
  assert(saved_smap);

  const auto& sect1 = saved_smap->get(3, 3);
  assert(sect1.get_condition() == SectorType::SEC_MOUNT);
  assert(!sect1.is_wasted());

  const auto& sect2 = saved_smap->get(4, 4);
  assert(sect2.get_condition() == SectorType::SEC_LAND);
  assert(!sect2.is_wasted());

  const auto& sect3 = saved_smap->get(5, 5);
  assert(sect3.get_condition() == SectorType::SEC_SEA);
  assert(!sect3.is_wasted());

  // Verify planet resources decreased
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  assert(saved_planet);
  assert(saved_planet->info(player_t{1}).resource ==
         1000 - (3 * SECTOR_REPAIR_COST));
}

void test_repair_scope_and_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Scope rejection at UNIV level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"repair"});
  assert(g.out.str().contains("Invalid scope for this command"));

  // 2. Domain error: no sectors owned on planet
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  {
    auto p_handle = ctx.em.get_planet(0, 0);
    p_handle->info(player_t{1}).numsectsowned = 0;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"repair", "3:5,3:5"});
  assert(g.out.str().contains("You don't own any sectors on this planet"));
}

}  // namespace

int main() {
  test_repair_happy_path();
  test_repair_scope_and_domain_errors();

  std::println(std::cout, "repair_test passed!");
  return 0;
}
