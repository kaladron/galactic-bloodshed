// SPDX-License-Identifier: Apache-2.0

/// \file toxicity_test.cc
/// \brief Unit tests for toxicity command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void test_toxicity_dispatch() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "Toxicologists";
  race.governor[0].active = true;
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star with AP
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.AP[0] = 10;
  Star star{star_data};
  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet
  Planet planet(PlanetType::EARTH);
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(player_t{1}).tox_thresh = 50;
  PlanetRepository planets(store);
  planets.save(planet);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Valid threshold update
  ctx.assert_dispatch_success(g, {"toxicity", "75"});
  assert(g.out.str().contains("New threshold is: 75"));
  auto saved = ctx.em.peek_planet(1, 0);
  assert(saved != nullptr);
  assert(saved->info(player_t{1}).tox_thresh == 75);
  std::println(std::cout, "    ✓ Set toxicity threshold to 75 succeeded");

  // 2. Reject illegal value (>100)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "150"});
  assert(g.out.str().contains("Illegal value"));
  std::println(std::cout, "    ✓ Illegal value 150 rejected");

  // 3. Reject missing arguments
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity"});
  assert(g.out.str().contains("Syntax: toxicity <threshold>"));
  std::println(std::cout,
               "    ✓ Missing argument rejected by descriptor min_args");

  // 4. Reject invalid scope (universal level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"toxicity", "50"});
  assert(g.out.str().contains("Invalid scope"));
  std::println(std::cout, "    ✓ Invalid scope rejected");
}

}  // namespace

int main() {
  test_toxicity_dispatch();
  std::println(std::cout, "\n✅ All toxicity tests passed!");
  return 0;
}
