// SPDX-License-Identifier: Apache-2.0

/// \file arm_test.cc
/// \brief Unit tests for arm and disarm commands.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void test_arm_and_disarm() {
  std::println(std::cout,
               "Test: arm and disarm command dispatch and domain logic");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Testers";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].money = 10000;
  race.fighters = 100;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.governor[0] = 0;
  star.AP[0] = 100;

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.info(player_t{1}).destruct = 1000;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet);
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(1000);
    smap.get(Coordinates{5, 5}).set_troops(0);
    smap.get(Coordinates{5, 5}).set_mobilization(1);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

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
  {
    auto guest_race_handle = ctx.em.get_race(1);
    guest_race_handle->Guest = true;
  }
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  {
    auto race_handle = ctx.em.get_race(1);
    race_handle->Guest = false;
  }
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
}

}  // namespace

int main() {
  test_arm_and_disarm();
  std::println(std::cout, "\n✅ All arm and disarm tests passed!");
  return 0;
}
