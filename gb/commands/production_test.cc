// SPDX-License-Identifier: Apache-2.0

/// \file production_test.cc
/// \brief Test production command functionality and reporting via
/// CommandDescriptor.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_production_dispatch() {
  std::println(std::cout, "Test: production command dispatch and reporting");

  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create universe
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Setup: Create star
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.explored = (1ULL << 1);
  ss0.inhabited = (1ULL << 1);
  ss0.pnames.push_back("Earth");
  Star star0(ss0);
  StarRepository stars(store);
  stars.save(star0);

  // Setup: Create planet
  Planet planet0{PlanetType::EARTH};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 10;
  planet0.info(player_t{1}).prod_res = 100;
  planet0.info(player_t{1}).prod_fuel = 50;
  planet0.info(player_t{1}).prod_dest = 20;
  planet0.info(player_t{1}).prod_crystals = 5;
  planet0.info(player_t{1}).est_production = 175.0;
  PlanetRepository planets(store);
  planets.save(planet0);

  // Setup: Create race
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  RaceRepository races(store);
  races.save(race1);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // 1. Production report for all stars (no args)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production"});
  std::string out = g.out.str();
  assert(out.contains("Production Report"));
  assert(out.contains("Sol/Eart"));
  std::println(std::cout, "    ✓ production all stars report succeeded");

  // 2. Production report for specific location
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/Sol"});
  out = g.out.str();
  assert(out.contains("Production Report"));
  assert(out.contains("Sol/Eart"));
  std::println(std::cout, "    ✓ production specific star report succeeded");

  // 3. Bad location warning
  g.out.str("");
  ctx.assert_dispatch_success(g, {"production", "/InvalidStar"});
  out = g.out.str();
  assert(out.contains("Bad location"));
  std::println(std::cout, "    ✓ production handled bad location cleanly");
}

}  // namespace

int main() {
  test_production_dispatch();

  std::println(std::cout, "All production tests passed!");
  return 0;
}
