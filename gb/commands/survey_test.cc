// SPDX-License-Identifier: Apache-2.0

/// \file survey_test.cc
/// \brief Unit tests for survey and client_survey commands

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 60.0;
  race.conditions[METHANE] = 5;
  race.conditions[OXYGEN] = 20;
  race.conditions[CO2] = 10;
  race.conditions[HYDROGEN] = 5;
  race.conditions[NITROGEN] = 50;
  race.conditions[SULFUR] = 5;
  race.conditions[HELIUM] = 3;
  race.conditions[OTHER] = 2;
  race.conditions[TEMP] = 280;

  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 0;
  star.name = "Sol";
  star.xpos = 100.0;
  star.ypos = 200.0;
  star.gravity = 1.0;
  star.stability = 45;
  star.temperature = 5;
  star.pnames.push_back("Earth");

  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 10.0;
  planet.ypos() = 20.0;
  planet.conditions(METHANE) = 5;
  planet.conditions(OXYGEN) = 20;
  planet.conditions(CO2) = 10;
  planet.conditions(HYDROGEN) = 5;
  planet.conditions(NITROGEN) = 50;
  planet.conditions(SULFUR) = 5;
  planet.conditions(HELIUM) = 3;
  planet.conditions(OTHER) = 2;
  planet.conditions(TEMP) = 280;
  planet.conditions(RTEMP) = 280;
  planet.conditions(TOXIC) = 15;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).fuel = 1000;
  planet.info(player_t{1}).resource = 2000;
  planet.info(player_t{1}).destruct = 100;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create sectors
  SectorMap smap(planet, true);
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
      auto& s = smap.get(x, y);
      s.set_condition(SectorType::SEC_LAND);
      s.set_type(SectorType::SEC_LAND);
      s.set_owner(1);
      s.set_race(1);
      s.set_efficiency_bounded(50);
      s.set_mobilization(10);
      s.set_fert(60);
      s.set_resource(40);
      s.set_troops(100);
      s.set_popn_exact(1000);
      s.set_crystals(false);
    }
  }

  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);
}

void test_survey_no_args_planet_scope() {
  std::println(std::cout, "Test: survey (no args) at planet scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "======== Planetary conditions: ========");
  test::expect_contains(out_str, "atmosphere concentrations");
  std::println(std::cout, "    ✓ Output contains planet survey information");
}

void test_survey_sector_range_with_header() {
  std::println(std::cout,
               "Test: survey command with sector range shows header");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"survey", "0:2,0:2"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "x,y");
  test::expect_contains(out_str, "cond/type");
  test::expect_contains(out_str, "owner");
  test::expect_contains(out_str, "xtals");
  test::expect_contains(out_str, "0,0");
  std::println(std::cout, "    ✓ Output contains header and sector data");
}

void test_survey_star_scope() {
  std::println(std::cout, "Test: survey command at star scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "Star Sol");
  test::expect_contains(out_str, "100,200");
  test::expect_contains(out_str, "Gravity");
  test::expect_contains(out_str, "Instability");
  test::expect_contains(out_str, "45%");
  test::expect_contains(out_str, "planets are");
  std::println(std::cout, "    ✓ Output contains star information");
}

void test_survey_universe_scope() {
  std::println(std::cout, "Test: survey command at universe scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "It's just _there_, you know?");
  std::println(std::cout, "    ✓ Universe scope survey succeeded");
}

void test_client_survey_dispatch() {
  std::println(std::cout, "Test: client_survey command dispatch");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"client_survey", "0:2,0:2"});
  test::expect_false(g.out.str().empty());
  std::println(std::cout, "    ✓ client_survey dispatched successfully");
}

}  // namespace

int main() {
  test_survey_no_args_planet_scope();
  test_survey_sector_range_with_header();
  test_survey_star_scope();
  test_survey_universe_scope();
  test_client_survey_dispatch();

  std::println(std::cout, "\n✅ All survey tests passed!");
  return 0;
}
