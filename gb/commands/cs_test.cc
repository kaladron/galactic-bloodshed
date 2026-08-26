// SPDX-License-Identifier: Apache-2.0

/// \file cs_test.cc
/// \brief Unit tests for cs command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  universe_struct us{};
  us.id = 1;
  us.numstars = 2;
  us.ships = 0;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].deflevel = ScopeLevel::LEVEL_STAR;
  race.governor[0].defsystem = 0;
  race.governor[0].defplanetnum = 0;
  RaceRepository races(store);
  races.save(race);

  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Alpha";
  ss0.xpos = 100.0;
  ss0.ypos = 200.0;
  ss0.pnames.emplace_back("AlphaPrime");
  ss0.explored = (1ULL << 1);
  Star star0(ss0);

  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Beta";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  ss1.explored = (1ULL << 1);
  Star star1(ss1);

  StarRepository stars_repo(store);
  stars_repo.save(star0);
  stars_repo.save(star1);

  Planet planet{PlanetType::EARTH, Coordinates{10, 10}};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 5;
  planet.Maxy() = 5;
  planet.explored() = true;
  planet.info(player_t{1}).explored = true;

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);
}

void test_cs_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Switch to universe scope (free AP)
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  ctx.assert_dispatch_success(g, {"cs", "/"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_UNIV);

  // 2. Switch to star Beta by name
  ctx.assert_dispatch_success(g, {"cs", "Beta"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.snum(), 1);

  // 3. Switch to planet AlphaPrime via full path
  ctx.assert_dispatch_success(g, {"cs", "/Alpha/AlphaPrime"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.snum(), 0);
  test::expect_eq(g.pnum(), 0);

  // 4. Default cs without arguments
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.snum(), 0);

  // 5. Change default system with -d
  ctx.assert_dispatch_success(g, {"cs", "-d", "/"}, 0);
  test::expect_contains(g.out.str(), "New home system");
}

void test_cs_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Invalid star name
  ctx.assert_dispatch_rejected(g, {"cs", "NonExistentStar"});
  test::expect_contains(g.out.str(), "cs: bad scope");
  test::expect_eq(g.level(), ScopeLevel::LEVEL_UNIV);

  // 2. Invalid home system format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-d", "NonExistentStar"});
  test::expect_contains(g.out.str(), "cs: bad home system");
}

}  // namespace

int main() {
  test_cs_happy_paths();
  test_cs_domain_errors();

  std::println(std::cout, "✓ cs_test passed!");
  return 0;
}
