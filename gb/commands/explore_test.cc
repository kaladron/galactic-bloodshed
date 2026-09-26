// SPDX-License-Identifier: Apache-2.0

/// \file explore_test.cc
/// \brief Unit tests for explore command

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
  us.AP[player_t{1}] = 50;  // Global AP for player 1
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Explorers";
  race.Guest = false;
  race.tech = 60.0;

  RaceRepository races(store);
  races.save(race);

  // Initialize star 1 (explored)
  Star star0{1, "Sol", {0.0, 0.0}};
  star0.stability() = 45;
  star0.mark_explored_by(1);
  star0.AP(1) = 20;
  star0.set_planet_name(1, "Earth");
  StarRepository stars(store);
  stars.save(star0);

  // Initialize planet 1 on star 1
  Planet planet0{1, 1, PlanetType::EARTH, Coordinates{10, 10}};
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 5;
  PlanetRepository planets(store);
  planets.save(planet0);

  // Initialize star 2 (unexplored)
  Star star1{2, "Centauri", {500.0, 500.0}};
  star1.stability() = 20;
  stars.save(star1);
}

void test_explore_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Happy path: explore without arguments (all explored stars)
  ctx.assert_dispatch_success(g, {"explore"});
  std::string output = g.out.str();
  test::expect_contains(output, "Exploration Report");
  test::expect_contains(output, "Sol");
  test::expect_contains(output, "Earth");
  test::expect_false(
      output.contains("Centauri"));  // Unexplored star should not appear
  std::println(std::cout, "    ✓ explore global census succeeded");

  // 2. Happy path: explore specific star
  g.out.str("");
  ctx.assert_dispatch_success(g, {"explore", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "Sol");
  test::expect_contains(output, "Earth");
  std::println(std::cout, "    ✓ explore /Sol succeeded");

  // 3. Bad scope rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"explore", "/NonExistentStar"});
  test::expect_true(g.out.str().contains("bad scope") ||
                    g.out.str().contains("No such star"));
  std::println(std::cout, "    ✓ explore rejected bad scope");
}

}  // namespace

int main() {
  test_explore_dispatch();

  std::println(std::cout, "All explore tests passed!");
  return 0;
}
