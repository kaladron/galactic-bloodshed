// SPDX-License-Identifier: Apache-2.0

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Testers", 100.0, false, player_t{1})
      .add_star("Test Star", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  // Set race likes
  {
    ctx.em.mutate_race(1, [](Race& r) {
      std::fill(std::begin(r.likes), std::end(r.likes), true);
    });
  }

  // Setup planet and sectormap
  ctx.em.mutate_planet(0, 0, [](Planet& p) { p.ships() = 1; });
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_MOUNT);
    smap.get(Coordinates{5, 6}).set_owner(1);
    smap.get(Coordinates{5, 6}).set_condition(SectorType::SEC_MOUNT);
  });

  // Create AFV ship landed at (5, 5)
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(1, 0)
      .named("AFV")
      .landed_on(0, 0, Coordinates(5, 5))
      .with_crew(10, 0)
      .with_fuel(100.0)
      .build();
}

void test_walk_role_and_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"walk", "1", "k"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // Restore non-guest race
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  // 2. Invalid ship rejection
  ctx.assert_dispatch_rejected(g, {"walk", "999", "k"});
  test::expect_contains(g.out.str(), "No such ship.");

  ctx.verify_universe_invariants();
}

void test_walk_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  // 3. Test walk command success - move south (k or '2')
  ctx.assert_dispatch_success(g, {"walk", "1", "k"});

  // Verify AFV moved and AP deducted
  ctx.em.clear_cache();
  const auto* saved_ship = ctx.em.peek_ship(1);
  test::expect_true(saved_ship != nullptr);
  test::expect_true(saved_ship->land_coords() == Coordinates(5, 6));
  test::expect_lt(saved_ship->fuel(), 100.0);

  const auto* saved_star = ctx.em.peek_star(0);
  test::expect_true(saved_star != nullptr);
  test::expect_eq(saved_star->AP(1), 99);  // 1 Star AP deducted

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_walk_role_and_domain_errors();
  test_walk_happy_path();

  std::println(std::cout, "✓ walk_test passed!");
  return 0;
}
