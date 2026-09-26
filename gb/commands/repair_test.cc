// SPDX-License-Identifier: Apache-2.0

/// \file repair_test.cc
/// \brief Unit tests for repair command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Testers")
      .add_star("Test Star", 10)
      .add_planet(1, PlanetType::EARTH, "Test Planet");

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{1}).resource = 1000;
  });

  // Populate test sectormap with wasted sectors
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{3, 3}).set_owner(1);
    smap.get(Coordinates{3, 3}).set_condition(SectorType::SEC_WASTED);
    smap.get(Coordinates{3, 3}).set_type(SectorType::SEC_MOUNT);
    smap.get(Coordinates{3, 3}).set_fert(50);

    smap.get(Coordinates{4, 4}).set_owner(1);
    smap.get(Coordinates{4, 4}).set_condition(SectorType::SEC_WASTED);
    smap.get(Coordinates{4, 4}).set_type(SectorType::SEC_LAND);
    smap.get(Coordinates{4, 4}).set_fert(30);

    smap.get(Coordinates{5, 5}).set_owner(0);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_WASTED);
    smap.get(Coordinates{5, 5}).set_type(SectorType::SEC_SEA);
    smap.get(Coordinates{5, 5}).set_fert(20);
  });
}

void test_repair_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"repair", "3:5,3:5"});
  test::expect_contains(g.out.str(), "3 sectors repaired at a cost of");

  // Verify sectors were repaired
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(1, 1);
  test::expect_ne(saved_smap, nullptr);

  const auto& sect1 = saved_smap->get(Coordinates{3, 3});
  test::expect_eq(sect1.get_condition(), SectorType::SEC_MOUNT);
  test::expect_false(sect1.is_wasted());

  const auto& sect2 = saved_smap->get(Coordinates{4, 4});
  test::expect_eq(sect2.get_condition(), SectorType::SEC_LAND);
  test::expect_false(sect2.is_wasted());

  const auto& sect3 = saved_smap->get(Coordinates{5, 5});
  test::expect_eq(sect3.get_condition(), SectorType::SEC_SEA);
  test::expect_false(sect3.is_wasted());

  // Verify planet resources decreased
  const auto* saved_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved_planet, nullptr);
  test::expect_eq(saved_planet->info(player_t{1}).resource,
                  1000 - (3 * SECTOR_REPAIR_COST));
}

void test_repair_scope_and_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Scope rejection at UNIV level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"repair"});
  test::expect_contains(g.out.str(), "Invalid scope for this command");

  // 2. Domain error: no sectors owned on planet
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.em.mutate_planet(
      1, 1, [](Planet& p) { p.info(player_t{1}).numsectsowned = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"repair", "3:5,3:5"});
  test::expect_contains(g.out.str(),
                        "You don't own any sectors on this planet");
}

}  // namespace

int main() {
  test_repair_happy_path();
  test_repair_scope_and_domain_errors();

  std::println(std::cout, "repair_test passed!");
  return 0;
}
