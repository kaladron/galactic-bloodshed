// SPDX-License-Identifier: Apache-2.0

/// \file tech_status_test.cc
/// \brief Unit tests for status (tech_status) command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Initialize universe
  universe_struct us{};
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Researchers";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);

  // Initialize power record for population total
  power p{};
  p.id = 1;
  p.popn = 10000;
  PowerRepository power_repo(store);
  power_repo.save(p);

  // Star 1
  Star star0{1, "Sol", {0.0, 0.0}};
  star0.mark_explored_by(1);
  star0.mark_inhabited_by(1);
  star0.governor(1) = 1;
  star0.set_planet_name(1, "Earth");

  StarRepository stars(store);
  stars.save(star0);

  // Planet 1 on Star 1
  Planet planet0{1, 1, PlanetType::EARTH, Coordinates{10, 10}};
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
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Happy path: status without arguments
  ctx.assert_dispatch_success(g, {"status"});
  std::string output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth");
  test::expect_contains(output, "10000");  // Population
  test::expect_contains(output, "50");     // Tech invest
  std::println(std::cout, "    ✓ status global colony report succeeded");

  // 2. Happy path: status with star location argument
  g.out.str("");
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth");
  std::println(std::cout, "    ✓ status with /Sol filter succeeded");
}

}  // namespace

int main() {
  test_status_dispatch();

  std::println(std::cout, "All status tests passed!");
  return 0;
}
