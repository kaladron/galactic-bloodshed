// SPDX-License-Identifier: Apache-2.0

/// \file tech_status_test.cc
/// \brief Unit tests for status (tech_status) command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Initialize universe
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Researchers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Initialize power record for population total
  power p{};
  p.id = 1;
  p.popn = 10000;
  PowerRepository power_repo(store);
  power_repo.save(p);

  // Star 0
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.explored = (1ULL << 1);
  ss0.inhabited = (1ULL << 1);
  ss0.governor[0] = 0;
  ss0.pnames.push_back("Earth");
  Star star0(ss0);

  StarRepository stars(store);
  stars.save(star0);

  // Planet 0 on Star 0
  Planet planet0{PlanetType::EARTH};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 10;
  planet0.info(player_t{1}).popn = 10000;
  planet0.info(player_t{1}).tech_invest = 50;
  planet0.info(player_t{1}).prod_res = 100;

  PlanetRepository planets(store);
  planets.save(planet0);
}

void test_status_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Happy path: status without arguments
  ctx.assert_dispatch_success(g, {"status"});
  std::string output = g.out.str();
  assert(output.contains("Technology Report"));
  assert(output.contains("Sol/Earth"));
  assert(output.contains("10000"));  // Population
  assert(output.contains("50"));     // Tech invest
  std::println(std::cout, "    ✓ status global colony report succeeded");

  // 2. Happy path: status with star location argument
  g.out.str("");
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  output = g.out.str();
  assert(output.contains("Technology Report"));
  assert(output.contains("Sol/Earth"));
  std::println(std::cout, "    ✓ status with /Sol filter succeeded");
}

}  // namespace

int main() {
  test_status_dispatch();

  std::println(std::cout, "All status tests passed!");
  return 0;
}
