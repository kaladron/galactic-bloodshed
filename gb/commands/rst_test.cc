// SPDX-License-Identifier: Apache-2.0

/// \file rst_test.cc
/// \brief Unit tests for ship reporting commands (report, ship, stats, stock,
/// weapons, factories)

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Universe
  universe_struct us{};
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Player race
  Race race{};
  race.Playernum = 1;
  race.name = "Admirals";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);

  // Star 1
  Star star0{1, "Sol", {0.0, 0.0}};
  star0.mark_explored_by(1);
  star0.mark_inhabited_by(1);
  star0.set_planet_name(1, "Earth");

  StarRepository stars(store);
  stars.save(star0);

  // Planet 1 on Star 1
  Planet planet0{1, 1, PlanetType::EARTH, Coordinates{10, 10}};
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 5;

  PlanetRepository planets(store);
  planets.save(planet0);

  // Ship 1: Shuttle in orbit of Sol
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1)
      .named("Hermes")
      .in_star_orbit(1)
      .with_crew(10, 0)
      .with_fuel(50.0)
      .with_max_fuel(100)
      .with_resource(20)
      .with_max_resource(100)
      .with_armor(5)
      .with_guns(guntype_t::HEAVY, 5)
      .with_destruct(10)
      .with_max_destruct(50)
      .build();

  // Ship 2: Factory ship on Earth
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 2)
      .owned_by(1)
      .named("Forge")
      .in_planet_orbit(1, 1)
      .with_build_type(ShipType::STYPE_FIGHTER)
      .with_build_cost(100)
      .with_crew(50, 0)
      .with_fuel(200.0)
      .with_max_fuel(500)
      .with_on(true)
      .build();
}

void test_rst_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. report command: summary of ships
  ctx.assert_dispatch_success(g, {"report"});
  std::string output = g.out.str();
  test::expect_true(output.contains("Hermes") || output.contains("#1"));
  std::println(std::cout, "    ✓ report command succeeded");

  // 2. ship command: full report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"ship"});
  output = g.out.str();
  test::expect_contains(output, "Hermes");
  std::println(std::cout, "    ✓ ship command succeeded");

  // 3. stats command: stats report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stats"});
  output = g.out.str();
  test::expect_contains(output, "Hermes");
  std::println(std::cout, "    ✓ stats command succeeded");

  // 4. stock command: cargo and inventory report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"stock"});
  output = g.out.str();
  test::expect_true(output.contains("res") || output.contains("fuel"));
  std::println(std::cout, "    ✓ stock command succeeded");

  // 5. weapons command: weapons report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"weapons"});
  output = g.out.str();
  test::expect_true(output.contains("guns") || output.contains("primary") ||
                    output.contains("Hermes"));
  std::println(std::cout, "    ✓ weapons command succeeded");

  // 6. factories command: factory report
  g.out.str("");
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_success(g, {"factories"});
  output = g.out.str();
  test::expect_true(output.contains("Cost") && output.contains("Weapons") &&
                    output.contains("100"));
  std::println(std::cout, "    ✓ factories command succeeded");

  // 7. Specific ship target: report #1
  g.out.str("");
  ctx.assert_dispatch_success(g, {"report", "#1"});
  test::expect_contains(g.out.str(), "Hermes");
  std::println(std::cout, "    ✓ report specific ship #1 succeeded");

  // 8. Specific ship letter filter: report s (shuttle)
  g.out.str("");
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.assert_dispatch_success(g, {"report", "s"});
  test::expect_contains(g.out.str(), "Hermes");
  std::println(std::cout, "    ✓ report shiptype filter succeeded");

  // 9. Error case: non-existent ship number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "#999"});
  test::expect_contains(g.out.str(), "no such ship");
  std::println(std::cout, "    ✓ report rejected non-existent ship");

  // 11. Spore Pod temperature suffix
  TestShipBuilder(ctx.em, ShipType::STYPE_POD, 3)
      .owned_by(1)
      .named("PodAlpha")
      .with_alive(true)
      .with_active(true)
      .in_star_orbit(1)
      .with_size(10)
      .with_pod(88)
      .build();

  g.out.str("");
  ctx.assert_dispatch_success(g, {"stats", "#3"});
  test::expect_contains(g.out.str(), "10 (88)");
  std::println(std::cout, "    ✓ pod temperature suffix displayed");
}

}  // namespace

int main() {
  test_rst_dispatch();

  std::println(std::cout, "All rst tests passed!");
  return 0;
}
