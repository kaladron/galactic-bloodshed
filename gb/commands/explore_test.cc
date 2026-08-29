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
  us.id = 1;
  us.numstars = 2;
  us.AP[player_t{1}] = 50;  // Global AP for player 1
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Explorers";
  race.Guest = false;
  race.tech = 60.0;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Initialize star 0 (explored)
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.stability = 45;
  ss0.explored = (1ULL << 1);  // Player 1 explored
  ss0.AP[0] = 20;
  ss0.pnames.push_back("Earth");
  Star star0(ss0);
  StarRepository stars(store);
  stars.save(star0);

  // Initialize planet 0 on star 0
  Planet planet0{PlanetType::EARTH, Coordinates{10, 10}};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 5;
  PlanetRepository planets(store);
  planets.save(planet0);

  // Initialize star 1 (unexplored)
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Centauri";
  ss1.xpos = 500.0;
  ss1.ypos = 500.0;
  ss1.stability = 20;
  ss1.explored = 0;
  Star star1(ss1);
  stars.save(star1);
}

void test_explore_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
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
