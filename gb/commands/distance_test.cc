// SPDX-License-Identifier: Apache-2.0

/// \file distance_test.cc
/// \brief Unit tests for distance command

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
  us.numstars = 2;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Rangers";
  race1.Guest = false;
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Aliens";
  race2.Guest = false;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Initialize star 0 at (0, 0)
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.explored = (1ULL << 1);
  Star star0(ss0);

  // Initialize star 1 at (300, 400) -> distance should be 500
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Centauri";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  ss1.explored = (1ULL << 1);
  Star star1(ss1);

  StarRepository stars(store);
  stars.save(star0);
  stars.save(star1);

  // Ships
  ShipRepository ships(store);

  // Ship 1: Player 1 at (0, 0)
  ship_struct s1{};
  s1.number = 1;
  s1.owner = 1;
  s1.type = ShipType::STYPE_SHUTTLE;
  s1.xpos = 0.0;
  s1.ypos = 0.0;
  s1.alive = 1;
  Ship ship1(s1);
  ships.save(ship1);

  // Ship 2: Player 1 at (30, 40) -> distance to ship 1 should be 50
  ship_struct s2{};
  s2.number = 2;
  s2.owner = 1;
  s2.type = ShipType::STYPE_SHUTTLE;
  s2.xpos = 30.0;
  s2.ypos = 40.0;
  s2.alive = 1;
  Ship ship2(s2);
  ships.save(ship2);

  // Ship 3: Player 2 (enemy)
  ship_struct s3{};
  s3.number = 3;
  s3.owner = 2;
  s3.type = ShipType::STYPE_SHUTTLE;
  s3.xpos = 100.0;
  s3.ypos = 100.0;
  s3.alive = 1;
  Ship ship3(s3);
  ships.save(ship3);
}

void test_distance_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args check: rejected when fewer than 3 args
  ctx.assert_dispatch_rejected(g, {"distance"});
  assert(g.out.str().contains("Syntax: distance <from> <to>"));
  std::println(std::cout,
               "    ✓ distance rejected with insufficient arguments");

  // 2. Happy path: distance between two stars (0,0) and (300,400) -> 500
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "/Sol", "/Centauri"});
  assert(g.out.str().contains("Distance = 500"));
  std::println(std::cout, "    ✓ distance between stars calculated accurately");

  // 3. Happy path: alias dist
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dist", "/Sol", "/Centauri"});
  assert(g.out.str().contains("Distance = 500"));
  std::println(std::cout, "    ✓ dist alias succeeded");

  // 4. Happy path: distance between two ships (0,0) and (30,40) -> 50
  g.out.str("");
  ctx.assert_dispatch_success(g, {"distance", "#1", "#2"});
  assert(g.out.str().contains("Distance = 50"));
  std::println(std::cout, "    ✓ distance between ships calculated accurately");

  // 5. Domain error: Foreign ship probe rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "#1", "#3"});
  assert(g.out.str().contains("Nice try"));
  std::println(std::cout, "    ✓ distance rejected query on foreign ship");

  // 6. Domain error: Bad scope
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"distance", "/NonExistentStar", "/Sol"});
  assert(g.out.str().contains("Bad scope") ||
         g.out.str().contains("No such star"));
  std::println(std::cout, "    ✓ distance rejected invalid scope");
}

}  // namespace

int main() {
  test_distance_dispatch();

  std::println(std::cout, "All distance tests passed!");
  return 0;
}
