// SPDX-License-Identifier: Apache-2.0

/// \file colonies_test.cc
/// \brief Test colonies command colonization report generation.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_colonies_dispatch() {
  std::println(std::cout, "Test: colonies command dispatch");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Initialize universe
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Setup test race
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  race1.governor[0].active = true;
  race1.conditions[0] = 50;

  RaceRepository races(store);
  races.save(race1);

  // Setup star and planet
  star_struct star_data{};
  star_data.star_id = 0;
  star_data.name = "Sol";
  star_data.governor[0] = 0;
  star_data.pnames.push_back("Earth");
  Star star{star_data};
  setbit<std::uint64_t>(star.explored(), 1U);
  StarRepository stars(store);
  stars.save(star);

  Planet planet(PlanetType::EARTH);
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.info(player_t{1}).explored = 1;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).popn = 1000;
  PlanetRepository planets(store);
  planets.save(planet);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Colonization report all stars
  ctx.assert_dispatch_success(g, {"colonies"});
  assert(g.out.str().contains("Colonization Report"));
  assert(g.out.str().contains("Sol"));
  std::println(std::cout,
               "    ✓ Colonization report across all stars succeeded");

  // 2. Colonization report for specific star
  g.out.str("");
  ctx.assert_dispatch_success(g, {"colonies", "/Sol"});
  assert(g.out.str().contains("Sol"));
  std::println(std::cout,
               "    ✓ Colonization report for specific star succeeded");
}

}  // namespace

int main() {
  test_colonies_dispatch();
  std::println(std::cout, "\n✅ All colonies tests passed!");
  return 0;
}
