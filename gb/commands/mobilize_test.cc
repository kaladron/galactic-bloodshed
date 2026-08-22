// SPDX-License-Identifier: Apache-2.0

/// \file mobilize_test.cc
/// \brief Unit tests for mobilize command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void test_mobilize_dispatch() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "Mobilizers";
  race.governor[0].active = true;
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;  // Player 1 governor 0 controls
  star_data.AP[0] = 10;       // Action points
  Star star{star_data};
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Setup: Create a planet
  Planet planet(PlanetType::EARTH);
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(player_t{1}).comread = 20;
  planet.info(player_t{1}).mob_set = 20;
  PlanetRepository planets(store);
  planets.save(planet);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Query mobilization (no args)
  ctx.assert_dispatch_success(g, {"mobilize"});
  assert(g.out.str().contains("Current mobilization: 20"));
  assert(g.out.str().contains("Quota: 20"));
  std::println(std::cout, "    ✓ Display current mobilization succeeded");

  // 2. Set mobilization to 50%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize", "50"});
  auto saved = ctx.em.peek_planet(1, 0);
  assert(saved != nullptr);
  assert(saved->info(player_t{1}).mob_set == 50);
  std::println(std::cout, "    ✓ Set mobilization to 50% succeeded");

  // 3. Set mobilization to 100%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"mobilize", "100"});
  saved = ctx.em.peek_planet(1, 0);
  assert(saved != nullptr);
  assert(saved->info(player_t{1}).mob_set == 100);
  std::println(std::cout, "    ✓ Set mobilization to 100% succeeded");

  // 4. Reject illegal value (>100)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "150"});
  assert(g.out.str().contains("Illegal value"));
  std::println(std::cout, "    ✓ Illegal value 150% rejected");

  // 5. Reject invalid scope (universal level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"mobilize", "50"});
  assert(g.out.str().contains("Invalid scope"));
  std::println(std::cout, "    ✓ Invalid scope rejected");
}

}  // namespace

int main() {
  test_mobilize_dispatch();
  std::println(std::cout, "\n✅ All mobilize tests passed!");
  return 0;
}
