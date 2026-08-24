// SPDX-License-Identifier: Apache-2.0

/// \file star_locations_test.cc
/// \brief Unit tests for stars (star_locations) command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Initialize universe
  universe_struct us{};
  us.id = 1;
  us.numstars = 2;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Initialize player race
  Race race{};
  race.Playernum = 1;
  race.name = "Astronomers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Star 0 at (0, 0)
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  Star star0(ss0);

  // Star 1 at (300, 400) -> distance = 500 from (0, 0)
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Vega";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  Star star1(ss1);

  StarRepository stars(store);
  stars.save(star0);
  stars.save(star1);
}

void test_stars_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.lastx[1] = 0.0;
  g.lasty[1] = 0.0;

  // 1. Happy path: stars without distance argument (lists all stars)
  ctx.assert_dispatch_success(g, {"stars"});
  std::string output = g.out.str();
  test::expect_contains(output, "Sol");
  test::expect_contains(output, "Vega");
  std::println(std::cout, "    ✓ stars listed all stellar positions");

  // 2. Happy path: stars with radius filter (should only include Sol)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "100"});
  output = g.out.str();
  test::expect_contains(output, "Sol");
  test::expect_false(output.contains("Vega"));
  std::println(std::cout, "    ✓ stars radius filter matched proximate star");

  // 3. Radius filter matching no stars if player is far away
  g.lastx[1] = 10000.0;
  g.lasty[1] = 10000.0;
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stars", "10"});
  test::expect_contains(g.out.str(),
                        "No stars found within specified distance.");
  std::println(std::cout, "    ✓ stars handled empty search radius cleanly");
}

}  // namespace

int main() {
  test_stars_dispatch();

  std::println(std::cout, "All stars tests passed!");
  return 0;
}
