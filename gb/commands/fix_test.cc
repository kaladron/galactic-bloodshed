// SPDX-License-Identifier: Apache-2.0

/// \file fix_test.cc
/// \brief Unit tests for fix command (deity utilities)

import dallib;
import dallib;
import gblib;
import commands;
import std;

#include <cassert>

// Test 1: Database persistence for fixing ship fuel
void test_fix_ship_fuel_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
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
  em.clear_cache();
  {
    const auto* s = em.peek_ship(1);
    assert(s);
    assert(s->fuel() == 50.0);
  }

  // 4. Simulate fixing fuel via EntityManager
  {
    auto ship_handle = em.get_ship(1);
    assert(ship_handle.get());
    auto& s = *ship_handle;
    s.fuel() = 200.0;  // Fill to max
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_ship = em.peek_ship(1);
  assert(final_ship);
  assert(final_ship->fuel() == 200.0);

  std::println("✓ fix ship fuel persistence test passed");
}

// Test 2: Database persistence for fixing ship damage
void test_fix_ship_damage_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
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
  em.clear_cache();
  {
    const auto* s = em.peek_ship(1);
    assert(s);
    assert(s->damage() == 75);
  }

  // 4. Simulate fixing damage via EntityManager
  {
    auto ship_handle = em.get_ship(1);
    assert(ship_handle.get());
    auto& s = *ship_handle;
    s.damage() = 0;  // Fully repair
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_ship = em.peek_ship(1);
  assert(final_ship);
  assert(final_ship->damage() == 0);

  std::println("✓ fix ship damage persistence test passed");
}

// Test 3: Database persistence for resurrecting ship
void test_fix_ship_alive_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
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
  em.clear_cache();
  {
    const auto* s = em.peek_ship(1);
    assert(s);
    assert(s->alive() == 0);
    assert(s->damage() == 100);
  }

  // 4. Simulate resurrecting ship via EntityManager
  {
    auto ship_handle = em.get_ship(1);
    assert(ship_handle.get());
    auto& s = *ship_handle;
    s.alive() = 1;
    s.damage() = 0;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_ship = em.peek_ship(1);
  assert(final_ship);
  assert(final_ship->alive() == 1);
  assert(final_ship->damage() == 0);

  std::println("✓ fix ship alive persistence test passed");
}

// Test 4: Database persistence for fixing planet temperature
void test_fix_planet_temp_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.conditions(TEMP) = 50;  // Initial temperature
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  em.clear_cache();
  {
    const auto* p = em.peek_planet(1, 0);
    assert(p);
    assert(p->conditions(TEMP) == 50);
  }

  // 4. Simulate fixing temperature via EntityManager
  {
    auto planet_handle = em.get_planet(1, 0);
    assert(planet_handle.get());
    auto& p = *planet_handle;
    p.conditions(TEMP) = 100;  // Set to 100
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_planet = em.peek_planet(1, 0);
  assert(final_planet);
  assert(final_planet->conditions(TEMP) == 100);

  std::println("✓ fix planet temperature persistence test passed");
}

// Test 5: Database persistence for fixing planet oxygen
void test_fix_planet_oxygen_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.conditions(OXYGEN) = 10;  // Initial oxygen
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  em.clear_cache();
  {
    const auto* p = em.peek_planet(1, 0);
    assert(p);
    assert(p->conditions(OXYGEN) == 10);
  }

  // 4. Simulate fixing oxygen via EntityManager
  {
    auto planet_handle = em.get_planet(1, 0);
    assert(planet_handle.get());
    auto& p = *planet_handle;
    p.conditions(OXYGEN) = 50;  // Increase oxygen
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_planet = em.peek_planet(1, 0);
  assert(final_planet);
  assert(final_planet->conditions(OXYGEN) == 50);

  std::println("✓ fix planet oxygen persistence test passed");
}

// Test 6: Database persistence for fixing planet position
void test_fix_planet_position_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository
  JsonStore store(db);
  PlanetRepository planets(store);

  // Create planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 100.0;
  planet.ypos() = 200.0;
  planets.save(planet);

  // 3. Verify initial state via EntityManager
  em.clear_cache();
  {
    const auto* p = em.peek_planet(1, 0);
    assert(p);
    assert(p->xpos() == 100.0);
    assert(p->ypos() == 200.0);
  }

  // 4. Simulate fixing position via EntityManager
  {
    auto planet_handle = em.get_planet(1, 0);
    assert(planet_handle.get());
    auto& p = *planet_handle;
    p.xpos() = 500.0;
    p.ypos() = 600.0;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_planet = em.peek_planet(1, 0);
  assert(final_planet);
  assert(final_planet->xpos() == 500.0);
  assert(final_planet->ypos() == 600.0);

  std::println("✓ fix planet position persistence test passed");
}

int main() {
  test_fix_ship_fuel_persistence();
  test_fix_ship_damage_persistence();
  test_fix_ship_alive_persistence();
  test_fix_planet_temp_persistence();
  test_fix_planet_oxygen_persistence();
  test_fix_planet_position_persistence();

  std::println("\n✅ All fix tests passed!");
  return 0;
}
