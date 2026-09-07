// SPDX-License-Identifier: Apache-2.0

/// \file arm_test.cc
/// \brief Unit tests for arm and disarm commands.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_arm_and_disarm() {
  std::println(std::cout,
               "Test: arm and disarm command dispatch and domain logic");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 100; });

  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned += 1;
    planet.info(player_t{1}).destruct = 1000;
    planet.popn() += 1000;
  });

  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_owner(1);
    sect.set_popn_exact(1000);
    sect.set_troops(0);
    sect.set_mobilization(1);
    sect.set_condition(SectorType::SEC_MOUNT);
  });

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Scope rejection at STAR scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "5,5", "50"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at star level verified");

  // 3. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);

  // 4. Test arm command success
  ctx.assert_dispatch_success(g, {"arm", "5,5", "100"});
  std::println(std::cout, "    ✓ Arm command succeeded");

  // Verify changes persisted
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(0, 0);
  test::expect_ne(saved_smap, nullptr);
  const auto& saved_sect = saved_smap->get(Coordinates{5, 5});

  test::expect_eq(saved_sect.get_troops(), 100);
  test::expect_eq(saved_sect.get_popn(), 900);

  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  test::expect_ne(saved_planet, nullptr);
  test::expect_eq(saved_planet->troops(), 100);

  const auto* saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->governor[0].money, 0);

  // 5. Test disarm command success
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_success(g, {"disarm", "5,5", "50"});
  std::println(std::cout, "    ✓ Disarm command succeeded");

  ctx.em.clear_cache();
  saved_smap = ctx.em.peek_sectormap(0, 0);
  const auto& saved_sect2 = saved_smap->get(Coordinates{5, 5});
  test::expect_eq(saved_sect2.get_troops(), 50);
  test::expect_eq(saved_sect2.get_popn(), 950);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_arm_and_disarm();
  std::println(std::cout, "\n✅ All arm and disarm tests passed!");
  return 0;
}
