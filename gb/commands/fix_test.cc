// SPDX-License-Identifier: Apache-2.0

/// \file fix_test.cc
/// \brief Unit tests for fix command (deity utilities)

import commands;
import dallib;
import gblib;
import test;
import std;

// Database persistence for fixing ship fuel
void test_fix_ship_fuel_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with low fuel
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.alive() = true;
  ship.fuel() = 50.0;
  ship.max_fuel() = 200;
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->fuel(), 50.0);
  }

  // 4. Simulate fixing fuel via EntityManager
  {
    auto ship_handle = ctx.em.get_ship(1);
    test::expect_ne(ship_handle.get(), nullptr);
    auto& s = *ship_handle;
    s.fuel() = 200.0;  // Fill to max
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->fuel(), 200.0);

  std::println(std::cout, "✓ fix ship fuel persistence test passed");
}

// Database persistence for fixing ship damage
void test_fix_ship_damage_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a damaged ship
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.alive() = true;
  ship.damage() = 75;
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->damage(), 75);
  }

  // 4. Simulate fixing damage via EntityManager
  {
    auto ship_handle = ctx.em.get_ship(1);
    test::expect_ne(ship_handle.get(), nullptr);
    auto& s = *ship_handle;
    s.damage() = 0;  // Fully repair
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->damage(), 0);

  std::println(std::cout, "✓ fix ship damage persistence test passed");
}

// Database persistence for resurrecting ship
void test_fix_ship_alive_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a dead ship
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.alive() = 0;
  ship.damage() = 100;
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->alive(), 0);
    test::expect_eq(s->damage(), 100);
  }

  // 4. Simulate resurrecting ship via EntityManager
  {
    auto ship_handle = ctx.em.get_ship(1);
    test::expect_ne(ship_handle.get(), nullptr);
    auto& s = *ship_handle;
    s.alive() = 1;
    s.damage() = 0;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->alive(), 1);
  test::expect_eq(final_ship->damage(), 0);

  std::println(std::cout, "✓ fix ship alive persistence test passed");
}

// Database persistence for fixing planet temperature
void test_fix_planet_temp_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.conditions(TEMP) = 50;  // Initial temperature
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 0);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->conditions(TEMP), 50);
  }

  // 4. Simulate fixing temperature via EntityManager
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    test::expect_ne(planet_handle.get(), nullptr);
    auto& p = *planet_handle;
    p.conditions(TEMP) = 100;  // Set to 100
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 0);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->conditions(TEMP), 100);

  std::println(std::cout, "✓ fix planet temperature persistence test passed");
}

// Database persistence for fixing planet oxygen
void test_fix_planet_oxygen_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.conditions(OXYGEN) = 10;  // Initial oxygen
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 0);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->conditions(OXYGEN), 10);
  }

  // 4. Simulate fixing oxygen via EntityManager
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    test::expect_ne(planet_handle.get(), nullptr);
    auto& p = *planet_handle;
    p.conditions(OXYGEN) = 50;  // Increase oxygen
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 0);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->conditions(OXYGEN), 50);

  std::println(std::cout, "✓ fix planet oxygen persistence test passed");
}

// Database persistence for fixing planet position
void test_fix_planet_position_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.xpos() = 100.0;
  planet.ypos() = 200.0;
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 0);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->xpos(), 100.0);
    test::expect_eq(p->ypos(), 200.0);
  }

  // 4. Simulate fixing position via EntityManager
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    test::expect_ne(planet_handle.get(), nullptr);
    auto& p = *planet_handle;
    p.xpos() = 500.0;
    p.ypos() = 600.0;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 0);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->xpos(), 500.0);
  test::expect_eq(final_planet->ypos(), 600.0);

  std::println(std::cout, "✓ fix planet position persistence test passed");
}

void test_fix_command_dispatch() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create deity race (player 1, deity = true)
  Race deity_race{};
  deity_race.Playernum = 1;
  deity_race.name = "Gods";
  deity_race.God = true;
  deity_race.Guest = false;

  // Create mortal race (player 2, deity = false)
  Race mortal_race{};
  mortal_race.Playernum = 2;
  mortal_race.name = "Mortals";
  mortal_race.God = false;
  mortal_race.Guest = false;

  RaceRepository races(store);
  races.save(deity_race);
  races.save(mortal_race);

  // Create star and planet
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "GodStar";
  StarRepository stars(store);
  stars.save(ss);

  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  planet.conditions(TEMP) = 50;
  PlanetRepository planets(store);
  planets.save(planet);

  // Create ship
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.alive() = true;
  ship.fuel() = 50.0;
  ship.max_fuel() = 200;
  ShipRepository ships(store);
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Mortal rejection
  ctx.setup_game_obj(g, 2, 0);
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "Only deity can use this command");

  // 2. Deity happy path - planet fix
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "TEMP = 100");

  // 3. Deity happy path - ship fix
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "fuel", "200"});
  test::expect_contains(g.out.str(), "fuel = 200");

  // 4. Min args check (< 3 args)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet"});
  test::expect_contains(g.out.str(),
                        "Syntax: fix <planet|ship> <property> [<value>]");
}

int main() {
  test_fix_ship_fuel_persistence();
  test_fix_ship_damage_persistence();
  test_fix_ship_alive_persistence();
  test_fix_planet_temp_persistence();
  test_fix_planet_oxygen_persistence();
  test_fix_planet_position_persistence();
  test_fix_command_dispatch();

  std::println(std::cout, "\n✅ All fix tests passed!");
  return 0;
}
