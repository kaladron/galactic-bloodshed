// SPDX-License-Identifier: Apache-2.0

/// \file tactical_test.cc
/// \brief Test tactical command functionality
///
/// This test verifies the standalone tactical.cc command works correctly.
/// The tactical command shows a combat display of ships and planets in the
/// current scope.

import commands;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

struct TestUniverseShips {
  player_t owner{2};
  player_t enemy{3};
  shipnum_t factory{0};
  shipnum_t destroyer{0};
  shipnum_t enemy_cruiser{0};
  shipnum_t enemy_tank{0};
  shipnum_t canister{0};
};

// Create a universe with combat ships and planets for tactical testing
TestUniverseShips setup_test_universe(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder(ctx)
      .add_race("Terrans", 100.0, false, 2)
      .add_race("Klingons", 100.0, false, 3);
  ctx.with_populated_planet(1, 1, 2, 1000, {0, 0});
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(2);
    s.mark_inhabited_by(2);
    s.AP(2) = 100;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.mark_explored_by(2);
    p.info(2).guns = 5;
  });

  ctx.em.mutate_race(2, [](Race& r) { r.declare_war_on(3); });

  TestUniverseShips ships;

  // Ship 1: Terran Factory at planet scope (has sight, no guns)
  ships.factory = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                      .owned_by(ships.owner)
                      .named("Factory1")
                      .in_planet_orbit(1, 1)
                      .build();

  // Ship 2: Terran Destroyer at same planet coordinates, armed,
  // moving/evading, laser focused
  {
    auto s2 = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                  .owned_by(ships.owner)
                  .named("Destroyer1")
                  .in_planet_orbit(1, 1)
                  .with_guns(guntype_t::MEDIUM, 4)
                  .with_crew(20, 0)
                  .with_speed(6)
                  .build_handle();
    s2->laser() = 1;
    s2->fire_laser() = 5;
    s2->focus() = 1;
    s2->navigate().on = 1;
    s2->protect().evade = true;
    ships.destroyer = s2->number();
  }

  // Ship 3: Enemy Cruiser at exact same planet coordinates (dist == 0)
  {
    auto s3 = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                  .owned_by(ships.enemy)
                  .named("EnemyCruiser")
                  .with_tech(80.0)
                  .in_planet_orbit(1, 1)
                  .with_speed(4)
                  .build_handle();
    s3->navigate().on = 1;
    s3->protect().evade = true;
    ships.enemy_cruiser = s3->number();
  }

  // Ship 4: Enemy Landed inactive AFV on Earth
  ships.enemy_tank = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                         .owned_by(ships.enemy)
                         .named("EnemyTank")
                         .with_active(false)
                         .with_radiation(50)
                         .landed_on(1, 1, {2, 3})
                         .build();

  // Ship 5: Canister (should be excluded from tactical target rows)
  ships.canister = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                       .owned_by(ships.enemy)
                       .named("DustCanister")
                       .in_planet_orbit(1, 1)
                       .build();

  return ships;
}

/// Test tactical at planet scope - shows ships and targets orbiting the planet
void test_tactical_planet_scope() {
  std::println(std::cout, "Test: Tactical at planet scope");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, ships.owner, 1);
  g_tactical.set_scope_planet(1, 1);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at planet scope");
  test::expect_contains(tactical_output, "Earth",
                        "Tactical at planet scope should show planet");
  // Verify same-coordinate enemy targets (dist == 0) are shown and canisters
  // excluded
  test::expect_contains(
      tactical_output, "EnemyCruiser",
      "Tactical should include enemy ship at same orbital coordinates");
  test::expect_contains(tactical_output, "EnemyTank",
                        "Tactical should include landed inactive enemy ship");
  test::expect_contains(tactical_output, "INACTIVE",
                        "Tactical should mark inactive target ship");
  test::expect_false(tactical_output.contains("DustCanister"),
                     "Tactical should exclude canisters from targets");

  std::println(
      std::cout,
      "  ✓ Planet scope produces tactical output with same-coordinate targets");
}

/// Test tactical at ship scope - shows surrounding area without duplicating the
/// scoped ship
void test_tactical_ship_scope() {
  std::println(std::cout, "Test: Tactical at ship scope");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, ships.owner, 1);
  g_tactical.set_scope_ship(ships.destroyer);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at ship scope");
  test::expect_contains(
      tactical_output, "Earth",
      "Tactical at ship scope should show surrounding planet");
  test::expect_contains(
      tactical_output, "EnemyCruiser",
      "Tactical at ship scope should show enemy ship in same orbit");

  // Ensure Destroyer1 header is printed only once (not duplicated)
  auto first_pos = tactical_output.find("Destroyer1");
  test::expect_true(first_pos != std::string::npos);
  auto second_pos = tactical_output.find("Destroyer1", first_pos + 1);
  test::expect_true(second_pos == std::string::npos,
                    "Scoped ship should not be reported twice");

  std::println(std::cout, "  ✓ Ship scope produces deduplicated tactical "
                          "output with surrounding area");
}

/// Test tactical at star scope - shows planets and ships in the star system
void test_tactical_star_scope() {
  std::println(std::cout, "Test: Tactical at star scope");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, ships.owner, 1);
  g_tactical.set_scope_star(1);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at star scope");
  test::expect_contains(tactical_output, "Earth",
                        "Tactical at star scope should show planet");

  std::println(std::cout, "  ✓ Star scope produces tactical output");
}

/// Test tactical explicit ship number, player filter, ship type filter, and
/// error paths
void test_tactical_explicit_ship_and_filters() {
  std::println(std::cout, "Test: Tactical explicit ship and filters");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);
  const std::string enemy_str = std::format("{}", ships.enemy.value);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, ships.owner, 1);
  g.set_scope_planet(1, 1);

  // Explicit ship + player filter: tactical #<destroyer> <enemy>
  const auto destroyer_arg = std::format("#{}", ships.destroyer);
  ctx.assert_dispatch_success(g, {"tactical", destroyer_arg, enemy_str});
  test::expect_contains(g.out.str(), "Destroyer1");
  test::expect_contains(g.out.str(), "EnemyCruiser");

  // Ship type filter + player filter: tactical C <enemy> (only Cruisers owned
  // by enemy)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tactical", "C", enemy_str});
  test::expect_contains(g.out.str(), "EnemyCruiser");
  test::expect_false(g.out.str().contains("EnemyTank"),
                     "Ship type filter 'C' should exclude AFVs");

  // Non-existent ship number: tactical #999
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tactical", "#999"});
  test::expect_contains(g.out.str(), "no such ship");

  // Malformed, trailing-garbage, #0, and overflowing ship numbers
  for (const auto& bad : {"#abc", "#12abc", "#99999999999999999999", "#"}) {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"tactical", bad});
    test::expect_contains(g.out.str(), "invalid ship argument");
  }
  ctx.assert_dispatch_rejected(g, {"tactical", "#0"});
  test::expect_contains(g.out.str(), "no such ship");

  // Sparse ship table: scope to factory before it is destroyed, verify #factory
  // argument fails with "no such ship" while higher-numbered ship succeeds, and
  // tactical with no args from destroyed scoped ship resets scope gracefully
  const auto high_ship = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                             .owned_by(ships.owner)
                             .named("HighIdShip")
                             .in_planet_orbit(1, 1)
                             .with_guns(guntype_t::MEDIUM, 2)
                             .with_crew(10, 0)
                             .build();
  ctx.em.mutate_ship(ships.factory, [&](Ship& s) { ctx.em.destroy_ship(s); });
  ctx.assert_dispatch_rejected(g,
                               {"tactical", std::format("#{}", ships.factory)});
  test::expect_contains(g.out.str(), "no such ship");
  ctx.assert_dispatch_success(g, {"tactical", std::format("#{}", high_ship)});
  test::expect_contains(g.out.str(), "HighIdShip");

  // Destroyed scoped ship resets scope on dispatch
  const auto doomed_ship = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                               .owned_by(ships.owner)
                               .in_planet_orbit(1, 1)
                               .build();
  g.set_scope_ship(doomed_ship);
  ctx.em.mutate_ship(doomed_ship, [&](Ship& s) { ctx.em.destroy_ship(s); });
  ctx.assert_dispatch_rejected(g, {"tactical"});
  test::expect_contains(g.out.str(), "no longer exists; scope reset");
  g.set_scope_planet(1, 1);

  // JSON mode output
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tactical", destroyer_arg, enemy_str});
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("name":"Destroyer1")");
  test::expect_contains(g.out.str(), R"("name":"EnemyCruiser")");

  std::println(std::cout,
               "  ✓ Explicit ship and filter arguments work properly");
}

void test_tactical_scope_rejection() {
  std::println(std::cout, "Test: Tactical scope rejection at UNIV scope");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, ships.owner, 1);
  g_tactical.set_scope_univ();

  ctx.assert_dispatch_rejected(g_tactical, {"tactical"});
  test::expect_contains(g_tactical.out.str(), "Invalid scope for this command");

  // Direct handler invocation at LEVEL_UNIV
  g_tactical.out.str("");
  test::expect_false(GB::commands::tactical({"tactical"}, g_tactical));
  test::expect_contains(g_tactical.out.str(),
                        "You can't do tactical from universe level");

  std::println(std::cout, "  ✓ Tactical rejected at universe level");
}

void test_tactical_combat_params_and_target_filtering() {
  std::println(std::cout,
               "Test: Tactical combat params, governor authorization, and "
               "target filtering");

  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder(ctx)
      .add_race("Terrans", 100.0, false, 2)
      .add_race("Klingons", 100.0, false, 3)
      .add_race("Romulans", 100.0, false, 4)
      .add_race("Cardassians", 100.0, false, 5);
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(2);
    s.mark_inhabited_by(2);
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.mark_explored_by(2); });

  ctx.em.mutate_race(2, [](Race& r) {
    r.declare_war_on(player_t{3});
    r.declare_alliance_with(player_t{4});
    r.appoint_governor(governor_t{2});
  });

  // Ship 1: Gov 2 firer with unfocused laser and destination-based movement
  {
    auto s1 = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                  .owned_by(player_t{2}, governor_t{2})
                  .named("UnfocusedFirer")
                  .in_planet_orbit(1, 1)
                  .with_guns(guntype_t::MEDIUM, 4)
                  .with_crew(20, 0)
                  .with_speed(6)
                  .targeting_planet(1, 1)
                  .build_handle();
    s1->laser() = 1;
    s1->fire_laser() = 5;
    s1->focus() = 0;
    s1->protect().evade = true;
  }

  // Ship 2: Gov 2 active landed firer (docked() == true, speed/evade
  // suppressed)
  {
    auto s2 = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                  .owned_by(player_t{2}, governor_t{2})
                  .named("LandedFirer")
                  .landed_on(1, 1, Coordinates{4, 5})
                  .with_guns(guntype_t::LIGHT, 2)
                  .with_crew(10, 0)
                  .with_speed(4)
                  .build_handle();
    s2->navigate().on = 1;
    s2->protect().evade = true;
  }

  // Ship 3: Gov 2 spaceborne inactive firer (!docked() && !active(), shows
  // INACTIVE(rad))
  {
    auto s3 = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                  .owned_by(player_t{2}, governor_t{2})
                  .named("InactiveFirer")
                  .in_planet_orbit(1, 1)
                  .with_guns(guntype_t::MEDIUM, 4)
                  .with_crew(20, 0)
                  .with_speed(6)
                  .with_active(false)
                  .with_radiation(40)
                  .build_handle();
    s3->navigate().on = 1;
    s3->protect().evade = true;
  }

  // Ship 4: Gov 2 uncrewed ship (has_sight() == false, skipped as firer)
  TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
      .owned_by(player_t{2}, governor_t{2})
      .named("BlindShip")
      .in_planet_orbit(1, 1)
      .with_crew(0, 0)
      .build();

  // Ship 5: Terrans Leader's ship (unauthorized for Gov 2 -> valid target, not
  // firer)
  TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
      .owned_by(player_t{2}, governor_t{1})
      .named("LeaderShip")
      .in_planet_orbit(1, 1)
      .with_crew(20, 0)
      .build();

  // Ship 6: Enemy target moving via destination (whatdest != LEVEL_UNIV,
  // navigate == 0)
  {
    auto s6 = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                  .owned_by(player_t{3}, governor_t{1})
                  .named("DestTarget")
                  .in_planet_orbit(1, 1)
                  .with_speed(5)
                  .targeting_planet(1, 1)
                  .build_handle();
    s6->protect().evade = true;
  }

  // Ship 7: Allied landed target (+, docked() == true, speed/evade suppressed)
  {
    auto s7 = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                  .owned_by(player_t{4}, governor_t{1})
                  .named("AlliedLanded")
                  .landed_on(1, 1, Coordinates{2, 2})
                  .with_speed(5)
                  .build_handle();
    s7->navigate().on = 1;
    s7->protect().evade = true;
  }

  // Ship 8: Neutral spaceborne target (' ') that is inactive (!docked() &&
  // !active())
  {
    auto s8 = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                  .owned_by(player_t{5}, governor_t{1})
                  .named("NeutralInact")
                  .in_planet_orbit(1, 1)
                  .with_speed(5)
                  .with_active(false)
                  .build_handle();
    s8->navigate().on = 1;
    s8->protect().evade = true;
  }

  // Ship 9: Greenhouse (excluded from tactical targets)
  TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
      .owned_by(player_t{3}, governor_t{1})
      .named("Greenhouse")
      .in_planet_orbit(1, 1)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{2}, governor_t{2});
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_success(g, {"tactical"});
  const std::string out = g.out.str();

  test::expect_contains(out, "UnfocusedFirer");
  test::expect_contains(out, "LandedFirer");
  test::expect_contains(out, "(4,5)");
  test::expect_contains(out, "INACTIVE(40)");
  test::expect_false(out.contains("BlindShip"),
                     "Uncrewed ship without sight should not report tactical");
  test::expect_contains(out, "LeaderShip",
                        "Same-race ship owned by another governor is a target");
  test::expect_contains(out, "DestTarget");
  test::expect_contains(out, "AlliedLanded");
  test::expect_contains(out, "+4,1");
  test::expect_contains(out, "NeutralInact");
  test::expect_false(out.contains("Greenhouse"),
                     "OTYPE_GREEN should be excluded from targets");

  std::println(std::cout, "  ✓ Combat params and target filtering verified");
}

void test_tactical_orbits_and_argument_edge_cases() {
  std::println(std::cout, "Test: Tactical orbital scopes and argument edges");

  TestContext ctx;
  const auto ships = setup_test_universe(ctx);

  // Ship 6: Terran ship in star orbit near Mars (200, 0), which has 0 sectors
  // owned and is unexplored by Terrans
  const auto s6_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                         .owned_by(ships.owner, governor_t{1})
                         .named("StarOrbitShip")
                         .in_star_orbit(1, SystemCoordinates{200.0, 0.0})
                         .with_guns(guntype_t::MEDIUM, 4)
                         .with_crew(20, 0)
                         .build();

  // Ship 7: Terran ship in deep space
  const auto s7_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                         .owned_by(ships.owner, governor_t{1})
                         .named("DeepSpaceShip")
                         .in_deep_space(UniverseCoordinates{500.0, 500.0})
                         .with_guns(guntype_t::MEDIUM, 4)
                         .with_crew(20, 0)
                         .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, ships.owner, governor_t{1});
  g.set_scope_planet(1, 1);

  // Numeric ship argument without '#' prefix
  ctx.assert_dispatch_success(g,
                              {"tactical", std::format("{}", ships.destroyer)});
  test::expect_contains(g.out.str(), "Destroyer1");

  // Multiple explicit ship arguments (star orbit + deep space)
  ctx.assert_dispatch_success(
      g, {"tactical", std::format("#{}", s6_id), std::format("#{}", s7_id)});
  test::expect_contains(g.out.str(), "StarOrbitShip");
  test::expect_contains(g.out.str(), "DeepSpaceShip");
  test::expect_contains(g.out.str(), "Mars");

  // Explicit ship + ship-type filter in argv[2]
  const auto destroyer_arg = std::format("#{}", ships.destroyer);
  ctx.assert_dispatch_success(g, {"tactical", destroyer_arg, "C"});
  test::expect_contains(g.out.str(), "EnemyCruiser");
  test::expect_false(g.out.str().contains("EnemyTank"));

  // Single ship-type filter argument (argv.size() == 2)
  ctx.assert_dispatch_success(g, {"tactical", "C"});
  test::expect_contains(g.out.str(), "EnemyCruiser");
  test::expect_false(g.out.str().contains("EnemyTank"));

  // Empty and malformed player filter strings
  ctx.assert_dispatch_success(g, {"tactical", "", ""});
  ctx.assert_dispatch_rejected(g, {"tactical", destroyer_arg, ""});
  for (const auto& bad_player : {"0", "2abc", "99999999999999999999"}) {
    ctx.assert_dispatch_rejected(g, {"tactical", "C", bad_player});
    test::expect_contains(g.out.str(), "No such player.");
    ctx.assert_dispatch_rejected(g, {"tactical", destroyer_arg, bad_player});
    test::expect_contains(g.out.str(), "No such player.");
  }
  ctx.assert_dispatch_rejected(g, {"tactical", "#0"});

  // Unexplored star at LEVEL_STAR: empty for mortal, populated for Deity;
  // stealthed orbital enemy mines and hangar craft are hidden from mortals,
  // and explicit #<foreign_ship> rejects without leaking existence.
  (void)TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
      .owned_by(ships.enemy, governor_t{1})
      .named("StealthMine")
      .in_planet_orbit(1, 1)
      .build();
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_success(g, {"tactical"});
  test::expect_false(g.out.str().contains("StealthMine"));
  ctx.assert_dispatch_rejected(
      g, {"tactical", std::format("#{}", ships.enemy_cruiser)});
  test::expect_contains(g.out.str(), "no such ship");

  g.set_scope_star(2);
  ctx.assert_dispatch_success(g, {"tactical"});
  test::expect_true(g.out.str().empty());
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"tactical"});
  test::expect_contains(g.out.str(), "Vega Prime");
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_success(g, {"tactical"});
  test::expect_contains(g.out.str(), "StealthMine");
  g.set_god(false);

  std::println(std::cout,
               "  ✓ Orbital scopes and argument edge cases verified");
}

}  // namespace

int main() {
  std::println(std::cout, "=== Tactical Command Test ===\n");

  test_tactical_planet_scope();
  test_tactical_ship_scope();
  test_tactical_star_scope();
  test_tactical_explicit_ship_and_filters();
  test_tactical_scope_rejection();
  test_tactical_combat_params_and_target_filtering();
  test_tactical_orbits_and_argument_edge_cases();

  std::println(std::cout, "\n✅ All tactical tests passed!");
  return 0;
}
