// SPDX-License-Identifier: Apache-2.0

/// \file orbit_test.cc
/// \brief Test orbit command graphic display and scope handling

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("TestRace", 100.0, false, player_t{1})
      .add_race("EnemyRace", 100.0, false, player_t{2});

  ctx.create_star("TestStar", starnum_t{1})
      .with_position(UniverseCoordinates{100.0, 200.0})
      .with_explored(player_t{1})
      .with_inhabited(player_t{1})
      .build();

  ctx.create_planet(1, PlanetType::EARTH, {10, 10}, planetnum_t{1})
      .named("TestPlanet")
      .with_explored(player_t{1}, true)
      .with_colony(player_t{1}, 1000)
      .build();

  // Setup: Create test ships at different scope levels
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 1)
      .named("TestFighter")
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1, 1)
      .named("Voyager")
      .in_deep_space(UniverseCoordinates{150.0, 250.0})
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1, 1)
      .named("OrbitShuttle")
      .in_planet_orbit(1, 1, UniverseCoordinates{105.0, 205.0})
      .build();
}

void test_orbit_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // TEST: Orbit display at star level
  std::println(std::cout, "Orbit command displays ship at star");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);

    ctx.assert_dispatch_success(g, {"orbit"});

    // Verify ships remain unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->owner(), player_t{1});
    test::expect_eq(saved_ship->whatorbits(), ScopeLevel::LEVEL_STAR);
    test::expect_eq(saved_ship->storbits(), 1);
    std::println(std::cout, "    ✓ Orbit display works correctly");
  }

  // TEST: Orbit -1 flag suppresses 1-based planet #1 and ship #1
  std::println(std::cout, "Orbit -1 flag suppresses object #1");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit", "-1", "/TestStar"});
    // Planet 1 (TestPlanet) and Ship #1 (f #1) must both be suppressed
    // when -1 is passed (verifies suppress_id is not decremented to 0).
    test::expect_false(g.out.str().contains("TestPlanet"));
    test::expect_false(g.out.str().contains("f #1"));
    std::println(std::cout,
                 "    ✓ Orbit -1 suppresses 1-based planet 1 and ship #1");
  }

  // TEST: Orbit display at planet level
  std::println(std::cout, "Orbit at planet level");
  {
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(1);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit"});

    std::string out = g.out.str();
    test::expect_contains(out, "TestPlanet");
    test::expect_contains(out, "s #3");
    std::println(std::cout, "    ✓ Planet-level orbit displays orbiting ships");
  }

  // TEST: Orbit options flags (-s, -p, -S) preserve star scope and filter
  std::println(std::cout, "Orbit options flags (-s, -p, -S)");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit", "-s"});
    std::string out_no_ships = g.out.str();
    test::expect_contains(out_no_ships, "TestStar");
    test::expect_contains(out_no_ships, "TestPlanet");
    test::expect_false(out_no_ships.contains("f #1"));

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit", "-p"});
    std::string out_no_planet_names = g.out.str();
    test::expect_contains(out_no_planet_names, "TestStar");
    test::expect_false(out_no_planet_names.contains("TestPlanet"));

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit", "-S"});
    std::string out_no_star_names = g.out.str();
    test::expect_false(out_no_star_names.contains("TestStar"));
    test::expect_contains(out_no_star_names, "TestPlanet");
    std::println(std::cout, "    ✓ Orbit -s, -p, -S options verified");
  }

  // TEST: Orbit inverse video standout and JSON mode
  std::println(std::cout, "Orbit inverse standout and JSON mode");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);

    ctx.em.mutate_race(1, [](Race& r) {
      r.governor(1).toggle.inverse = true;
      r.governor(1).toggle.highlight = player_t{1};
      r.governor(1).toggle.compat = true;
    });
    g.race = ctx.em.peek_race(1);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit"});
    std::string inv_out = g.out.str();
    test::expect_contains(inv_out, "\x1b[7m*\x1b[27m");
    test::expect_contains(inv_out, "\x1b[7mf #1\x1b[27m");

    g.set_ui_mode(UiMode::JSON);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit"});
    std::string json_out = g.out.str();
    test::expect_contains(json_out, "\"type\":\"orbit\"");
    test::expect_contains(json_out, "\"label\":\"TestStar\"");
    test::expect_contains(json_out, "\"label\":\"TestPlanet\"");
    test::expect_contains(json_out, "\"shipno\":1");
    g.set_ui_mode(UiMode::ASCII);
    std::println(std::cout,
                 "    ✓ Orbit inverse standout and JSON output verified");
  }

  // TEST: Orbit display at universe level
  std::println(std::cout, "Orbit at universe level");
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);

    ctx.assert_dispatch_success(g, {"orbit"});

    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->owner(), player_t{1});

    const auto* saved_ship2 = ctx.em.peek_ship(2);
    test::expect_ne(saved_ship2, nullptr);
    test::expect_eq(saved_ship2->owner(), player_t{1});
    test::expect_eq(saved_ship2->whatorbits(), ScopeLevel::LEVEL_UNIV);
    std::println(
        std::cout,
        "    ✓ Universe-level orbit displays stars and ships in transit");
  }
}

void test_orbit_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Invalid option number format
  ctx.assert_dispatch_rejected(g, {"orbit", "-abc"});
  test::expect_contains(g.out.str(), "Bad number");

  // 2. Invalid target scope path
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"orbit", "nonexistent/star"});
  test::expect_contains(g.out.str(), "orbit: error in args.");
  // 3. Ship scope is rejected with "Bad scope."
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"orbit", "#1"});
  test::expect_contains(g.out.str(), "Bad scope.");
}

void test_orbit_visibility_and_filtering() {
  TestContext ctx;
  setup_test_world(ctx);

  // Add a second unexplored planet in star 1
  ctx.create_planet(1, PlanetType::GASGIANT, {10, 10}, planetnum_t{2})
      .named("HiddenWorld")
      .with_position(SystemCoordinates{50.0, -50.0})
      .with_explored(player_t{1}, false)
      .build();

  // Add canister, greenhouse, friendly mine, enemy mine, enemy fighter in star
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST, 10)
      .owned_by(1, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN, 11)
      .owned_by(1, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_MINE, 12)
      .owned_by(1, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_MINE, 13)
      .owned_by(2, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 14)
      .owned_by(2, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
      .build();

  // Add out-of-bounds ships (negative projected X and negative projected Y)
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 15)
      .owned_by(1, 1)
      .in_star_orbit(1, UniverseCoordinates{-50000.0, 200.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 16)
      .owned_by(1, 1)
      .in_star_orbit(1, UniverseCoordinates{100.0, -50000.0})
      .build();

  // Add enemy deep-space ship (#17), landed planet ship (#18), and enemy
  // planet-orbiting ship (#19)
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 17)
      .owned_by(2, 1)
      .in_deep_space(UniverseCoordinates{150.0, 250.0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 18)
      .owned_by(1, 1)
      .landed_on(1, 1, {0, 0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 19)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1, UniverseCoordinates{105.0, 205.0})
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Star scope with sight (ship #1 has crew): sees friendly mine (#12) and
  // enemy fighter (#14), hides canister (#10), green (#11), enemy mine (#13),
  // and out-of-bounds ships (#15, #16). Unexplored planet 2 renders '?' symbol.
  ctx.em.mutate_ship(1, [](Ship& s) { s.popn() = 1; });
  ctx.em.mutate_ship(3, [](Ship& s) { s.popn() = 1; });
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  std::string star_out = g.out.str();
  test::expect_contains(star_out, "? HiddenWorld");
  test::expect_contains(star_out, "+ #12");
  test::expect_contains(star_out, "f #14");
  test::expect_false(star_out.contains("#10"));
  test::expect_false(star_out.contains("#11"));
  test::expect_false(star_out.contains("#13"));
  test::expect_false(star_out.contains("#15"));
  test::expect_false(star_out.contains("#16"));

  // 2. Star scope without sight (player 2 has no crewed ship if #14 is
  // uncrewed): player 1 with ship #1 uncrewed cannot see enemy #14 unless god.
  ctx.em.mutate_ship(1, [](Ship& s) { s.popn() = 0; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_false(g.out.str().contains("f #14"));

  // God mode restores sight in star system without crewed ships
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "f #14");
  g.set_god(false);

  // 3. Planet scope: landed ship (#18) is hidden; enemy ship (#19) is visible
  // when #3 has sight, hidden when #3 has no crew, and -s / -3 flags work.
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "s #3");
  test::expect_contains(g.out.str(), "f #19");
  test::expect_false(g.out.str().contains("#18"));

  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit", "-3"});
  test::expect_false(g.out.str().contains("s #3"));

  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit", "-s"});
  test::expect_false(g.out.str().contains("s #3"));

  ctx.em.mutate_ship(3, [](Ship& s) { s.popn() = 0; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_false(g.out.str().contains("f #19"));

  // 4. Universe scope: enemy deep-space ship (#17) hidden unless god; -1 and
  // -s flags filter universe stars and ships.
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "C #2");
  test::expect_false(g.out.str().contains("#17"));

  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "C #17");
  g.set_god(false);

  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit", "-1", "-s"});
  test::expect_false(g.out.str().contains("TestStar"));
  test::expect_false(g.out.str().contains("C #2"));

  std::println(std::cout,
               "    ✓ Orbit visibility, sight IQ, and filtering verified");
}

void test_orbit_space_mirror_aiming() {
  TestContext ctx;
  setup_test_world(ctx);

  // Space Mirror direction projection
  const auto mirror_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR, 4)
          .owned_by(1)
          .named("SolarMirror")
          .with_alive(true)
          .with_active(true)
          .in_star_orbit(1, UniverseCoordinates{100.0, 200.0})
          .with_aim(AimedAtData{
              .shipno = std::nullopt,
              .snum = starnum_t{1},
              .intensity = 5,
              .pnum = planetnum_t{1},
              .level = ScopeLevel::LEVEL_PLAN,
          })
          .build();

  const auto* ship = ctx.em.peek_ship(mirror_id);
  test::expect_ne(ship, nullptr);
  const auto* mirror = ship->as<SpaceMirrorShip>();
  test::expect_ne(mirror, nullptr);

  // 1. Target coordinates for planet: Star (100, 200) + Planet (0, 0)
  auto coords = ctx.em.resolve_mirror_target_coordinates(*mirror);
  test::expect_true(coords.has_value());
  test::expect_eq(coords->x, 100.0);
  test::expect_eq(coords->y, 200.0);

  // 2. Aim at a ship located at (150.0, 250.0) -> south-east heading
  ctx.em.mutate_as<SpaceMirrorShip>(
      mirror_id, [](SpaceMirrorShip& m) { m.aim_at_ship(2); });

  const auto* updated_mirror =
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  auto ship_coords = ctx.em.resolve_mirror_target_coordinates(*updated_mirror);
  test::expect_true(ship_coords.has_value());
  test::expect_eq(ship_coords->x, 150.0);
  test::expect_eq(ship_coords->y, 250.0);

  const bearing_t dir = ctx.em.resolve_mirror_aim_direction(*updated_mirror);
  test::expect_eq(dir, 3u);  // slope = +1.0, dy > 0 -> direction 3

  // 3. Verify orbit command includes the mirror in system view
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "M #4 (aim: 3)");

  std::println(std::cout, "    ✓ Space mirror targeting, direction heading, "
                          "and orbit display verified");
}

}  // namespace

int main() {
  test_orbit_happy_path();
  test_orbit_domain_errors();
  test_orbit_visibility_and_filtering();
  test_orbit_space_mirror_aiming();

  std::println(std::cout, "\n✅ All orbit tests passed!");
  return 0;
}
