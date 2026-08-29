// SPDX-License-Identifier: Apache-2.0

/// \file tax_test.cc
/// \brief Unit tests for tax command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Test querying and setting planetary tax rate successfully
void test_tax_happy_paths() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Gov_ship = 100;
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[player_t{1}] = 0;  // Player 1, Governor 0 controls star

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tax = 10;
  planet.info(1).newtax = 10;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
    StarRepository stars(store);
    stars.save(star);
    PlanetRepository planets(store);
    planets.save(planet);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Query current tax rate
  ctx.assert_dispatch_success(g, {"tax"});
  test::expect_contains(g.out.str(), "Current tax rate: 10%");
  test::expect_contains(g.out.str(), "Target: 10%");

  // 2. Set new tax rate to 25%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "25"});
  test::expect_contains(g.out.str(), "Set.");
  test::expect_eq(ctx.em.peek_planet(1, 0)->info(1).newtax, 25);

  // 3. Set new tax rate to 100% (max)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "100"});
  test::expect_eq(ctx.em.peek_planet(1, 0)->info(1).newtax, 100);

  // 4. Set new tax rate to 0% (min)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "0"});
  test::expect_eq(ctx.em.peek_planet(1, 0)->info(1).newtax, 0);
}

// Test tax command role and scope rejections
void test_tax_role_and_scope_rejections() {
  TestContext ctx;
  Race normal_race{};
  normal_race.Playernum = 1;
  normal_race.name = "NormalRace";
  normal_race.Gov_ship = 100;
  normal_race.Guest = false;

  Race guest_race{};
  guest_race.Playernum = 2;
  guest_race.name = "GuestRace";
  guest_race.Gov_ship = 100;
  guest_race.Guest = true;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[player_t{1}] = 0;  // Player 1 controls star, Player 2 does not

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tax = 10;
  planet.info(2).tax = 10;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(normal_race);
    races.save(guest_race);
    StarRepository stars(store);
    stars.save(star);
    PlanetRepository planets(store);
    planets.save(planet);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 2, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Star control rejection (Governor 2 on star assigned to Governor 1)
  ctx.em.mutate_star(1, [](Star& s) {
    s.governor(1) = 1;  // Star assigned to Governor 1
  });
  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);  // Player 1, Governor 2
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system.");

  // 3. Scope rejection (ScopeLevel::LEVEL_UNIV)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_tax_domain_errors() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Gov_ship = 0;  // No government center
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[player_t{1}] = 0;

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tax = 10;
  planet.info(1).newtax = 10;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
    StarRepository stars(store);
    stars.save(star);
    PlanetRepository planets(store);
    planets.save(planet);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Domain error: No government center active
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "You have no government center active.");

  // 2. Domain error: Illegal value (>100 or <0)
  ctx.em.mutate_race(1, [](Race& r) { r.Gov_ship = 100; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "150"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(1, 0)->info(1).newtax, 10);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "-10"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(1, 0)->info(1).newtax, 10);
}

}  // namespace

int main() {
  test_tax_happy_paths();
  test_tax_role_and_scope_rejections();
  test_tax_domain_errors();

  std::println(std::cout, "✓ tax_test passed!");
  return 0;
}
