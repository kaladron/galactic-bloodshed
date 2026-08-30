// SPDX-License-Identifier: Apache-2.0

/// \file orbit_test.cc
/// \brief Test orbit command graphic display and scope handling

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Setup: Create universe with 1 star and ships
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  us.ships = 3;

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Setup: Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Setup: Create a test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.pnames.push_back("TestPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Setup: Create a test planet
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.ships = 3;  // First ship in planet ship list
  ps.info[player_t{1}].explored = true;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Setup: Create test ships at different scope levels
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.active() = true;
  ship1.type() = ShipType::STYPE_FIGHTER;
  ship1.name() = "TestFighter";
  ship1.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship1.storbits() = 0;
  ship1.xpos() = 100.0;
  ship1.ypos() = 200.0;

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_CRUISER;
  ship2.name() = "Voyager";
  ship2.whatorbits() = ScopeLevel::LEVEL_UNIV;
  ship2.xpos() = 150.0;
  ship2.ypos() = 250.0;

  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 1;
  ship3.governor() = 0;
  ship3.alive() = true;
  ship3.active() = true;
  ship3.type() = ShipType::STYPE_SHUTTLE;
  ship3.name() = "OrbitShuttle";
  ship3.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship3.storbits() = 0;
  ship3.pnumorbits() = 0;
  ship3.xpos() = 105.0;
  ship3.ypos() = 205.0;
  ship3.docked() = false;  // Orbiting, not landed

  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
  ships_repo.save(ship3);
}

void test_orbit_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // TEST: Orbit display at star level
  std::println(std::cout, "Orbit command displays ship at star");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    ctx.assert_dispatch_success(g, {"orbit"});

    // Verify ships remain unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->owner(), player_t{1});
    test::expect_eq(saved_ship->whatorbits(), ScopeLevel::LEVEL_STAR);
    test::expect_eq(saved_ship->storbits(), 0);
    std::println(std::cout, "    ✓ Orbit display works correctly");
  }

  // TEST: Orbit display at planet level
  std::println(std::cout, "Orbit at planet level");
  {
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(0);
    g.set_pnum(0);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit"});

    std::string out = g.out.str();
    test::expect_false(out.empty());
    std::println(std::cout, "    ✓ Planet-level orbit displays orbiting ships");
  }

  // TEST: Orbit options flags (-s to suppress ships)
  std::println(std::cout, "Orbit options flags (-s, -p)");
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    g.out.str("");
    ctx.assert_dispatch_success(g, {"orbit", "-s"});
    std::string out_no_ships = g.out.str();

    test::expect_false(out_no_ships.empty());
    std::println(std::cout, "    ✓ Orbit -s option executed cleanly");
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
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Invalid option number format
  ctx.assert_dispatch_rejected(g, {"orbit", "-abc"});
  test::expect_contains(g.out.str(), "Bad number");

  // 2. Invalid target scope path
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"orbit", "nonexistent/star"});
  test::expect_contains(g.out.str(), "orbit: error in args.");
}

void test_orbit_space_mirror_aiming() {
  TestContext ctx;
  setup_test_world(ctx);

  // Setup a space mirror ship at position (100.0, 200.0)
  ship_struct mirror_data{};
  mirror_data.number = 10;
  mirror_data.owner = 1;
  mirror_data.governor = 0;
  mirror_data.alive = true;
  mirror_data.active = true;
  mirror_data.type = ShipType::STYPE_MIRROR;
  mirror_data.name = "SolarMirror";
  mirror_data.whatorbits = ScopeLevel::LEVEL_STAR;
  mirror_data.storbits = 0;
  mirror_data.xpos = 100.0;
  mirror_data.ypos = 200.0;
  mirror_data.special = AimedAtData{
      .shipno = shipnum_t{0},
      .snum = starnum_t{0},
      .intensity = 5,
      .pnum = planetnum_t{0},
      .level = ScopeLevel::LEVEL_PLAN,
  };
  auto mirror_handle = ctx.em.create_ship(mirror_data);
  const auto mirror_id = mirror_handle->number();

  const auto* ship = ctx.em.peek_ship(mirror_id);
  test::expect_ne(ship, nullptr);
  const auto* mirror = ship->as<SpaceMirrorShip>();
  test::expect_ne(mirror, nullptr);

  // 1. Target coordinates for planet: Star (100, 200) + Planet (0, 0)
  auto coords = mirror->target_coordinates(ctx.em);
  test::expect_true(coords.has_value());
  test::expect_eq(coords->first, 100.0);
  test::expect_eq(coords->second, 200.0);

  // 2. Aim at a ship located at (150.0, 250.0) -> south-east heading
  ctx.em.mutate_as<SpaceMirrorShip>(mirror_id, [](SpaceMirrorShip& m) {
    m.aim().level = ScopeLevel::LEVEL_SHIP;
    m.aim().shipno = 2;
  });

  const auto* updated_mirror =
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  auto ship_coords = updated_mirror->target_coordinates(ctx.em);
  test::expect_true(ship_coords.has_value());
  test::expect_eq(ship_coords->first, 150.0);
  test::expect_eq(ship_coords->second, 250.0);

  int dir = updated_mirror->aim_direction(ctx.em);
  test::expect_eq(dir, 3);  // slope = +1.0, dy > 0 -> direction 3

  // 3. Verify orbit command includes the mirror in system view
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  g.out.str("");
  ctx.assert_dispatch_success(g, {"orbit"});
  test::expect_contains(g.out.str(), "3 M false");

  std::println(std::cout, "    ✓ Space mirror targeting, direction heading, "
                          "and orbit display verified");
}

}  // namespace

int main() {
  test_orbit_happy_path();
  test_orbit_domain_errors();
  test_orbit_space_mirror_aiming();

  std::println(std::cout, "\n✅ All orbit tests passed!");
  return 0;
}
