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
  us.id = 1;
  us.numstars = 1;
  us.ships = 2;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Player race
  Race race{};
  race.Playernum = 1;
  race.name = "Admirals";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Star 0
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.explored = (1ULL << 1);
  ss0.inhabited = (1ULL << 1);
  ss0.ships = 1;
  ss0.pnames.push_back("Earth");
  Star star0(ss0);

  StarRepository stars(store);
  stars.save(star0);

  // Planet 0 on Star 0
  Planet planet0{PlanetType::EARTH, Coordinates{10, 10}};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.ships() = 2;
  planet0.info(player_t{1}).explored = 1;
  planet0.info(player_t{1}).numsectsowned = 5;

  PlanetRepository planets(store);
  planets.save(planet0);

  // Ships
  ShipRepository ships(store);

  // Ship 1: Shuttle in orbit of Sol
  ship_struct s1{};
  s1.number = 1;
  s1.owner = 1;
  s1.type = ShipType::STYPE_SHUTTLE;
  s1.name = "Hermes";
  s1.alive = 1;
  s1.active = 1;
  s1.whatorbits = ScopeLevel::LEVEL_STAR;
  s1.storbits = 0;
  s1.popn = 10;
  s1.fuel = 50.0;
  s1.max_fuel = 100;
  s1.resource = 20;
  s1.max_resource = 100;
  s1.armor = 5;
  s1.guns = 1;
  s1.primtype = guntype_t::GTYPE_HEAVY;
  s1.primary = 5;
  s1.destruct = 10;
  s1.max_destruct = 50;
  Ship ship1(s1);
  ships.save(ship1);

  // Ship 2: Factory ship on Earth
  ship_struct s2{};
  s2.number = 2;
  s2.owner = 1;
  s2.type = ShipType::OTYPE_FACTORY;
  s2.name = "Forge";
  s2.alive = 1;
  s2.active = 1;
  s2.whatorbits = ScopeLevel::LEVEL_PLAN;
  s2.storbits = 0;
  s2.pnumorbits = 0;
  s2.popn = 50;
  s2.fuel = 200.0;
  s2.max_fuel = 500;
  s2.build_type = ShipType::STYPE_FIGHTER;
  s2.build_cost = 100;
  s2.on = 1;
  Ship ship2(s2);
  ships.save(ship2);
}

void test_rst_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

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
  g.set_snum(0);
  g.set_pnum(0);
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
  g.set_snum(0);
  ctx.assert_dispatch_success(g, {"report", "s"});
  test::expect_contains(g.out.str(), "Hermes");
  std::println(std::cout, "    ✓ report shiptype filter succeeded");

  // 9. Error case: non-existent ship number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "#999"});
  test::expect_contains(g.out.str(), "no such ship");
  std::println(std::cout, "    ✓ report rejected non-existent ship");

  // 10. Error case: invalid ship letter
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "?"});
  test::expect_contains(g.out.str(), "no valid ship letters found");
  std::println(std::cout, "    ✓ report rejected invalid ship letters");
}

}  // namespace

int main() {
  test_rst_dispatch();

  std::println(std::cout, "All rst tests passed!");
  return 0;
}
