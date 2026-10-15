// SPDX-License-Identifier: Apache-2.0

/// \file explore_test.cc
/// \brief Unit tests for explore command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Explorers", 60.0, false, player_t{1})
      .add_race("Aliens", 50.0, false, player_t{2});

  ctx.em.mutate_universe([](Universe& univ) { univ.set_AP(player_t{1}, 50); });

  // Star 1 (explored): Planet 1 (explored, inhabited, autorep, enslaved, toxic,
  // alien owner) and Planet 2 (unexplored)
  ctx.create_star("Sol", starnum_t{1})
      .with_position(UniverseCoordinates{0.0, 0.0})
      .with_stability(45)
      .with_explored(player_t{1})
      .with_ap(player_t{1}, 20)
      .with_planet_names({"Earth", "Mars"})
      .build();

  ctx.create_planet(starnum_t{1}, PlanetType::EARTH, Coordinates{10, 10},
                    planetnum_t{1})
      .named("Earth")
      .with_explored(player_t{1}, true)
      .with_enslaved_to(player_t{2})
      .with_toxicity(80)
      .with_colony(player_t{1}, 1000, Coordinates{0, 0})
      .with_sector(Coordinates{1, 0}, SectorType::SEC_LAND, 100, 100, 100,
                   player_t{2}, 500)
      .build();

  ctx.em.mutate_planet(starnum_t{1}, planetnum_t{1}, [](Planet& earth) {
    earth.info(player_t{1}).autorep = 1;
    earth.info(player_t{2}).numsectsowned = 1;
  });

  ctx.create_planet(starnum_t{1}, PlanetType::MARS, Coordinates{10, 10},
                    planetnum_t{2})
      .named("Mars")
      .with_explored(player_t{1}, false)
      .build();

  // Star 2 (unexplored)
  ctx.create_star("Centauri", starnum_t{2})
      .with_position(UniverseCoordinates{500.0, 500.0})
      .with_stability(20)
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, shipnum_t{1})
      .owned_by(player_t{1}, governor_t{1})
      .in_star_orbit(starnum_t{1})
      .build();
}

void test_explore_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Happy path: explore without arguments (all explored stars, high tech)
  ctx.assert_dispatch_success(g, {"explore"});
  std::string output = g.out.str();
  test::expect_contains(output, "Exploration Report");
  test::expect_contains(output, "Sol (45)[20]");
  test::expect_contains(output, "Earth");
  test::expect_contains(output, "Ex Rep Inhab SLAVED 2 TOXIC");
  test::expect_contains(output, "Mars");
  test::expect_contains(output, "No Data");
  test::expect_false(output.contains("Centauri"));
  std::println(std::cout, "    ✓ explore global census succeeded");

  // 2. Low tech: stability hidden as (/?/?)
  ctx.em.mutate_race(player_t{1}, [](Race& race) { race.tech = 10.0; });
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"explore", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "Sol (/?/?)[20]");
  test::expect_contains(output, "Earth");
  std::println(std::cout, "    ✓ explore /Sol low-tech stability succeeded");

  // 3. JSON mode rendering
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"explore", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "\"type\":\"explore\"");
  test::expect_contains(output, "\"star_name\":\"Sol\"");
  test::expect_contains(output, "\"planet_name\":\"Earth\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ explore JSON mode succeeded");

  // 4. Bad scope rejection (non-existent star, universe scope '/', ship '#1')
  ctx.assert_dispatch_rejected(g, {"explore", "/NonExistentStar"});
  test::expect_true(g.out.str().contains("bad scope") ||
                    g.out.str().contains("No such star"));

  ctx.assert_dispatch_rejected(g, {"explore", "/"});
  test::expect_contains(g.out.str(), "Bad scope '/'");

  ctx.assert_dispatch_rejected(g, {"explore", "#1"});
  test::expect_contains(g.out.str(), "Bad scope '#1'");
  std::println(std::cout, "    ✓ explore rejected invalid scopes");
}

}  // namespace

int main() {
  test_explore_dispatch();

  std::println(std::cout, "All explore tests passed!");
  return 0;
}
