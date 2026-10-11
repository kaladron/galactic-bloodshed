// SPDX-License-Identifier: Apache-2.0

/// \file rst_test.cc
/// \brief Unit tests for ship reporting commands (report, ship, stats, stock,
/// weapons, factories)

import commands;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx).add_race("Admirals").add_race("Rivals");

  ctx.create_star("Sol", 1)
      .with_explored(1)
      .with_inhabited(1)
      .with_planet_name(1, "Earth")
      .build();

  ctx.create_planet(1, PlanetType::EARTH, {10, 10}, 1)
      .with_explored(player_t{1})
      .build();

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
  test::expect_true(output.contains("Hermes") && output.contains("Forge"));
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
  test::expect_true(output.contains("res") && output.contains("fuel"));
  std::println(std::cout, "    ✓ stock command succeeded");

  // 5. weapons command: weapons report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"weapons"});
  output = g.out.str();
  test::expect_true(output.contains("guns") && output.contains("Hermes"));
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

  // 7. Specific ship target: report #1 and numeric "1"
  g.out.str("");
  ctx.assert_dispatch_success(g, {"report", "#1"});
  test::expect_contains(g.out.str(), "Hermes");
  g.out.str("");
  ctx.assert_dispatch_success(g, {"report", "1"});
  test::expect_contains(g.out.str(), "Hermes");
  std::println(std::cout, "    ✓ report specific ship #1 succeeded");

  // 8. Specific ship letter filter: report s (shuttle)
  g.out.str("");
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.assert_dispatch_success(g, {"report", "s"});
  test::expect_contains(g.out.str(), "Hermes");
  std::println(std::cout, "    ✓ report shiptype filter succeeded");

  // 9. Error cases: non-existent ship number, #0, invalid scan, invalid letter
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "#999"});
  test::expect_contains(g.out.str(), "no such ship");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "#0"});
  test::expect_contains(g.out.str(), "no such ship");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "#abc"});
  test::expect_contains(g.out.str(), "invalid ship argument");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"report", "?"});
  test::expect_contains(g.out.str(), "no valid ship letters found");
  std::println(std::cout, "    ✓ report error cases rejected");

  // 10. Spore Pod temperature suffix
  TestShipBuilder(ctx.em, ShipType::STYPE_POD, 3)
      .owned_by(1)
      .named("PodAlpha")
      .in_star_orbit(1)
      .with_size(10)
      .with_pod(88)
      .build();

  g.out.str("");
  ctx.assert_dispatch_success(g, {"stats", "#3"});
  test::expect_contains(g.out.str(), "10 (88)");
  std::println(std::cout, "    ✓ pod temperature suffix displayed");
}

void test_rst_filters_scopes_and_json() {
  TestContext ctx;
  setup_test_world(ctx);

  // Add ships covering Terra, Plow, unconfigured factories, canisters,
  // greenhouses, dead/foreign/governor-restricted ships, and carrier cargo.
  TestShipBuilder(ctx.em, ShipType::OTYPE_TERRA, 3)
      .owned_by(1)
      .named("Terraformer")
      .in_deep_space({5.0, 5.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_PLOW, 4)
      .owned_by(1)
      .named("SpacePlow")
      .in_star_orbit(1)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 5)
      .owned_by(1)
      .named("UnconfPod")
      .in_star_orbit(1)
      .with_build_type(ShipType::STYPE_POD)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 6)
      .owned_by(1)
      .named("UnconfFac")
      .in_star_orbit(1)
      .with_build_type(ShipType::OTYPE_FACTORY)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST, 7)
      .owned_by(1)
      .named("FreeCan")
      .in_star_orbit(1)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST, 8)
      .owned_by(1)
      .named("LandedCan")
      .landed_on(1, 1, {3, 4})
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN, 9)
      .owned_by(1)
      .named("FreeGreen")
      .in_star_orbit(1)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN, 10)
      .owned_by(1)
      .named("DockedGreen")
      .docked_to(1, 1)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 11)
      .owned_by(1)
      .named("IrradiatedShip")
      .with_active(false)
      .with_radiation(50)
      .in_star_orbit(1)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 12)
      .owned_by(2)
      .named("EnemyShip")
      .in_star_orbit(1)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 13)
      .owned_by(1, 3)
      .named("OtherGovShip")
      .in_star_orbit(1)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // Universe scope report for governor 1 (skips FreeCan, FreeGreen,
  // EnemyShip; includes Terraformer, LandedCan, INACTIVE(50)).
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"report"});
  const std::string univ_out = g.out.str();
  test::expect_contains(univ_out, "Terraformer");
  test::expect_contains(univ_out, "LandedCan");
  test::expect_contains(univ_out, "L3,4");
  test::expect_contains(univ_out, "INACTIVE(50)");
  test::expect_true(!univ_out.contains("FreeCan"));
  test::expect_true(!univ_out.contains("FreeGreen"));
  test::expect_true(!univ_out.contains("EnemyShip"));

  // Subordinate governor 2 skips governor 3's ship
  g.out.str("");
  g.set_governor(2);
  ctx.assert_dispatch_success(g, {"report", "#13"});
  test::expect_true(!g.out.str().contains("OtherGovShip"));
  g.set_governor(1);

  // Weapons and factories on Terra, Plow, and unconfigured factories
  g.out.str("");
  ctx.assert_dispatch_success(g, {"weapons", "#3", "#4"});
  test::expect_contains(g.out.str(), "Standard");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"factories", "#5", "#6"});
  test::expect_contains(g.out.str(), "(No ship type specified yet)");

  // Ship scope: unscoped (shipno == 0) fails; scoped to #1 reports #1 + #10
  g.out.str("");
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(0);
  ctx.assert_dispatch_rejected(g, {"report"});
  test::expect_contains(g.out.str(), "No ship is currently scoped");

  g.out.str("");
  g.set_shipno(1);
  ctx.assert_dispatch_success(g, {"report"});
  test::expect_contains(g.out.str(), "Hermes");
  test::expect_contains(g.out.str(), "DockedGreen");

  // JSON mode output
  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"report", "#1"});
  const std::string json_out = g.out.str();
  test::expect_contains(json_out, "\"type\":\"rst\"");
  test::expect_contains(json_out, "\"name\":\"Hermes\"");
}

}  // namespace

int main() {
  test_rst_dispatch();
  test_rst_filters_scopes_and_json();

  std::println(std::cout, "All rst tests passed!");
  return 0;
}
