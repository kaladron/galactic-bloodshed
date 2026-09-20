// SPDX-License-Identifier: Apache-2.0

/// \file fix_test.cc
/// \brief Unit tests for fix command (deity utilities)

import commands;
import dallib;
import gb.entities;
import gb.services;
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
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 0)
      .with_max_fuel(200.0)
      .with_fuel(50.0)
      .build();

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->fuel(), 50.0);
  }

  // 4. Simulate fixing fuel via EntityManager
  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(200.0); });

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
  ship.admin_override_damage(75);
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->damage(), 75);
  }

  // 4. Simulate fixing damage via EntityManager
  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_damage(0); });

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->damage(), 0);

  std::println(std::cout, "✓ fix ship damage persistence test passed");
}

// Database persistence for fixing ship alive status
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
  ship.alive() = false;
  ship.admin_override_damage(100);
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
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.alive() = 1;
    s.admin_override_damage(0);
  });

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
  ctx.em.mutate_planet(1, 0, [](Planet& p) { p.conditions(TEMP) = 100; });

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
  ctx.em.mutate_planet(1, 0, [](Planet& p) { p.conditions(OXYGEN) = 50; });

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
  planet.set_system_coordinates({100.0, 200.0});
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 0);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->system_coordinates(), SystemCoordinates{100.0, 200.0});
  }

  // 4. Simulate fixing position via EntityManager
  ctx.em.mutate_planet(
      1, 0, [](Planet& p) { p.set_system_coordinates({500.0, 600.0}); });

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 0);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->system_coordinates(),
                  SystemCoordinates{500.0, 600.0});

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
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 0)
      .with_max_fuel(200.0)
      .with_fuel(50.0)
      .build();

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
  test::expect_contains(g.out.str(), "temperature = 100");

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

  // 5. Unknown target ("Fix what?")
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "star", "temperature"});
  test::expect_contains(g.out.str(), "Fix what?");

  // 6. Planet scope error & invalid numeric value & all condition options
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "Change scope to the planet first.");

  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "abc"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "nonexistent", "10"});
  test::expect_contains(g.out.str(), "No such option for 'fix planet'.");

  // Exercise xpos, ypos, and all condition options (set + inspect)
  ctx.assert_dispatch_success(g, {"fix", "planet", "xpos", "250"});
  test::expect_contains(g.out.str(), "xpos = 250");
  ctx.assert_dispatch_success(g, {"fix", "planet", "ypos", "-125"});
  test::expect_contains(g.out.str(), "ypos = -125");

  for (Conditions cond : all_condition_types) {
    const std::string opt{to_string(cond)};
    const int val = static_cast<int>(cond) + 10;
    g.out.str("");
    ctx.assert_dispatch_success(g, {"fix", "planet", opt, std::to_string(val)});
    test::expect_contains(g.out.str(), std::format("{} = {}", cond, val));

    // Read-only inspection without value argument
    g.out.str("");
    ctx.assert_dispatch_success(g, {"fix", "planet", opt});
    test::expect_contains(g.out.str(), std::format("{} = {}", cond, val));
  }

  // 7. Ship scope error, invalid numeric value, and all ship options
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "fuel", "100"});
  test::expect_contains(g.out.str(),
                        "Change scope to the ship you wish to fix.");

  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "fuel", "notanumber"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "shields", "10"});
  test::expect_contains(g.out.str(), "No such option for 'fix ship'.");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "max_fuel", "350"});
  test::expect_contains(g.out.str(), "fuel = 350");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "destruct", "2"});
  test::expect_contains(g.out.str(), "destruct = 2");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "resource", "20"});
  test::expect_contains(g.out.str(), "resource = 20");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "damage", "35"});
  test::expect_contains(g.out.str(), "damage = 35");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "dead"});
  test::expect_contains(g.out.str(), "destroyed");
  test::expect_false(ctx.em.peek_ship(1)->alive());

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "alive"});
  test::expect_contains(g.out.str(), "resurrected");
  test::expect_true(ctx.em.peek_ship(1)->alive());
  test::expect_eq(ctx.em.peek_ship(1)->damage(), 0);
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
