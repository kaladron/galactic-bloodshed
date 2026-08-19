// SPDX-License-Identifier: Apache-2.0

/// \file technology_test.cc
/// \brief Unit tests for technology command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

// Test querying and setting planetary technology investment successfully
void test_technology_happy_paths() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[0] = 0;  // Player 1, Governor 0 controls star
  star.AP[0] = 10;

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tech_invest = 100;
  planet.info(1).popn = 1000;

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

  // 1. Query current technology investment (costs 1 AP)
  ctx.assert_dispatch_success(g, {"technology"}, 1);
  assert(g.out.str().contains("Current investment : 100"));
  assert(g.out.str().contains("Technology production/update:"));

  // 2. Set technology investment to 500 (costs 1 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"technology", "500"}, 1);
  assert(g.out.str().contains("New (ideal) tech production:"));
  assert(ctx.em.peek_planet(1, 0)->info(1).tech_invest == 500);

  // 3. Set technology investment to 0
  g.out.str("");
  ctx.assert_dispatch_success(g, {"technology", "0"}, 1);
  assert(ctx.em.peek_planet(1, 0)->info(1).tech_invest == 0);
}

// Test technology command with insufficient AP
void test_technology_insufficient_ap() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[0] = 0;
  star.AP[0] = 0;  // 0 AP (needs 1)

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tech_invest = 100;
  planet.info(1).popn = 1000;

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

  ctx.assert_dispatch_rejected(g, {"technology", "500"});
  assert(g.out.str().contains("You don't have 1 action points there."));
  assert(ctx.em.peek_planet(1, 0)->info(1).tech_invest == 100);
}

// Test technology command role and scope rejections
void test_technology_role_and_scope_rejections() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[0] = 1;  // Assigned to Governor 1
  star.AP[0] = 10;

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tech_invest = 100;
  planet.info(1).popn = 1000;

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

  // 1. Star control rejection (Governor 2 on star assigned to Governor 1)
  ctx.setup_game_obj(g, 1, 2);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"technology", "200"});
  assert(g.out.str().contains(
      "You are not authorized to do that in this system."));

  // 2. Scope rejection (ScopeLevel::LEVEL_UNIV)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"technology", "200"});
  assert(g.out.str().contains("Invalid scope for this command."));
}

// Test technology command domain logic errors
void test_technology_domain_errors() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.governor[0] = 0;
  star.AP[0] = 10;

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(1).tech_invest = 100;
  planet.info(1).popn = 1000;

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

  // Domain error: Illegal negative value (0 AP deducted, investment unchanged)
  ctx.assert_dispatch_rejected(g, {"technology", "-100"});
  assert(g.out.str().contains("Illegal value."));
  assert(ctx.em.peek_planet(1, 0)->info(1).tech_invest == 100);
}

}  // namespace

int main() {
  test_technology_happy_paths();
  test_technology_insufficient_ap();
  test_technology_role_and_scope_rejections();
  test_technology_domain_errors();

  std::println(std::cout, "✓ technology_test passed!");
  return 0;
}
