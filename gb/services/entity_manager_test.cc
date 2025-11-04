// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>

void test_entity_manager_basic() {
  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);

  EntityManager em(db);

  std::println("Test: EntityManager basic functionality");

  // Create and save a race
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.tech = 50.0;

  // Use RaceRepository directly to save initial data
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Get race through EntityManager
  {
    auto handle = em.get_race(1);
    assert(handle.get() != nullptr);
    assert(handle->name == "Test Race");
    assert(handle->tech == 50.0);

    // Modify the race
    handle->tech = 75.0;
    // Auto-saves on scope exit
  }

  // Verify the modification was persisted
  auto saved_race = races.find_by_player(1);
  assert(saved_race.has_value());
  assert(saved_race->tech == 75.0);

  std::println("  ✓ Basic get/modify/auto-save works");
}

void test_entity_manager_caching() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager caching");

  // Create a ship
  Ship ship{};
  ship.number = 100;
  ship.owner = 1;
  ship.fuel = 1000.0;

  JsonStore store(db);
  ShipRepository ships(store);
  ships.save(ship);

  // Get ship multiple times
  const Ship* first_ptr = nullptr;
  {
    auto handle1 = em.get_ship(100);
    first_ptr = handle1.get();
    assert(first_ptr != nullptr);

    // Get same ship again - should return same cached instance
    auto handle2 = em.get_ship(100);
    assert(handle2.get() == first_ptr);

    std::println("  ✓ Multiple get calls return same cached instance");

    // Modify via one handle
    handle1->fuel = 500.0;

    // Verify modification visible via other handle (same instance)
    assert(handle2->fuel == 500.0);

    std::println("  ✓ Modifications visible across all handles (same instance)");
  }

  // After all handles released, cache should be cleared
  // Get ship again - might be reloaded or cached depending on refcount
  auto handle3 = em.get_ship(100);
  assert(handle3.get() != nullptr);
  assert(handle3->fuel == 500.0);  // Persisted value

  std::println("  ✓ Entity persists after cache clear");
}

void test_entity_manager_composite_keys() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager composite keys (Planet)");

  // Create a planet
  Planet planet{};
  planet.star_id = 5;
  planet.planet_order = 2;
  planet.Maxx = 10;
  planet.Maxy = 10;

  JsonStore store(db);
  PlanetRepository planets(store);
  planets.save(planet);

  // Get planet using composite key
  {
    auto handle1 = em.get_planet(5, 2);
    assert(handle1.get() != nullptr);
    assert(handle1->planet_order == 2);
    assert(handle1->Maxx == 10);

    // Modify the planet
    handle1->Maxx = 15;

    // Get the same planet again - should return the SAME in-memory instance
    auto handle2 = em.get_planet(5, 2);
    assert(handle2.get() != nullptr);
    assert(handle2.get() == handle1.get());  // Same pointer!
    assert(handle2->Maxx == 15);  // Sees the modification immediately

    std::println("  ✓ Multiple handles to same planet return identical instance");
    // Handles go out of scope here, triggering save
  }

  // Verify modification was persisted to database
  auto saved_planet = planets.find_by_location(5, 2);
  assert(saved_planet.has_value());
  assert(saved_planet->Maxx == 15);

  std::println("  ✓ Composite keys work for Planet entities");
}

void test_entity_manager_create_delete() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager create/delete");

  // Create a new ship
  shipnum_t ship_num;
  {
    auto handle = em.create_ship();
    assert(handle.get() != nullptr);
    ship_num = handle->number;

    handle->owner = 1;
    handle->fuel = 2000.0;
    // Auto-saves on scope exit
  }

  // Verify ship was created and saved
  JsonStore store(db);
  ShipRepository ships(store);
  auto loaded_ship = ships.find_by_number(ship_num);
  assert(loaded_ship.has_value());
  assert(loaded_ship->fuel == 2000.0);

  std::println("  ✓ create_ship() creates and saves new ship");

  // Delete the ship
  em.delete_ship(ship_num);

  // Verify deletion
  auto deleted_ship = ships.find_by_number(ship_num);
  assert(!deleted_ship.has_value());

  std::println("  ✓ delete_ship() removes ship from cache and database");
}

void test_entity_manager_read_only_access() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager read-only access");

  // Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "Observer";
  race.tech = 100.0;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Load into cache and keep handle alive
  auto handle = em.get_race(1);
  assert(handle.get() != nullptr);

  // Use peek for read-only access while handle is alive
  const Race* race_ptr = em.peek_race(1);
  assert(race_ptr != nullptr);
  assert(race_ptr == handle.get());  // Same instance!
  assert(race_ptr->name == "Observer");
  assert(race_ptr->tech == 100.0);

  std::println("  ✓ peek_race() provides read-only access to cached entity");

  // Peek non-existent entity
  const Race* null_ptr = em.peek_race(999);
  assert(null_ptr == nullptr);

  std::println("  ✓ peek_*() returns nullptr for non-cached entities");
}

void test_entity_manager_flush_all() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager flush_all");

  // Create multiple entities
  JsonStore store(db);
  RaceRepository races(store);
  ShipRepository ships(store);

  Race race{};
  race.Playernum = 1;
  race.tech = 50.0;
  races.save(race);

  Ship ship{};
  ship.number = 100;
  ship.fuel = 1000.0;
  ships.save(ship);

  // Load and modify both
  {
    auto race_handle = em.get_race(1);
    auto ship_handle = em.get_ship(100);

    race_handle->tech = 75.0;
    ship_handle->fuel = 500.0;

    // Don't let handles go out of scope yet
    // Force flush while handles still exist
    em.flush_all();

    // Verify immediate persistence
    auto saved_race = races.find_by_player(1);
    auto saved_ship = ships.find_by_number(100);

    assert(saved_race.has_value());
    assert(saved_race->tech == 75.0);
    assert(saved_ship.has_value());
    assert(saved_ship->fuel == 500.0);

    std::println("  ✓ flush_all() saves all cached entities immediately");
  }
}

void test_entity_manager_clear_cache() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager clear_cache");

  // Create and cache an entity
  Race race{};
  race.Playernum = 1;
  race.tech = 50.0;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Load into cache and keep handle alive
  {
    auto handle = em.get_race(1);
    assert(handle.get() != nullptr);

    // Verify it's in cache
    const Race* cached = em.peek_race(1);
    assert(cached != nullptr);

    // Clear cache (but entity stays because handle is alive)
    em.clear_cache();

    // Peek still returns the entity because handle is alive
    const Race* still_there = em.peek_race(1);
    assert(still_there != nullptr);

    std::println("  ✓ clear_cache() preserves entities with active handles");
    // Handle goes out of scope here
  }

  // Now verify peek reloads from database (even if cache was empty)
  const Race* after_clear = em.peek_race(1);
  assert(after_clear != nullptr);
  assert(after_clear->tech == 50.0);

  std::println("  ✓ peek_* reloads from database after cache clear");

  // Can still load from database
  auto handle2 = em.get_race(1);
  assert(handle2.get() != nullptr);
  assert(handle2->tech == 50.0);

  std::println("  ✓ Entities can be reloaded after cache clear");
}

void test_entity_manager_singleton_stardata() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager singleton (stardata)");

  // Create initial stardata
  stardata sd{};
  sd.id = 1;  // Stardata is a singleton with id=1
  sd.numstars = 100;
  sd.ships = 50;

  JsonStore store(db);
  StardataRepository stardata_repo(store);
  stardata_repo.save(sd);

  // Get stardata (singleton)
  {
    auto handle = em.get_stardata();
    assert(handle.get() != nullptr);
    assert(handle->numstars == 100);

    // Modify
    handle->numstars = 150;
  }

  // Verify modification persisted
  auto saved = stardata_repo.get_global_data();
  assert(saved.has_value());
  assert(saved->numstars == 150);

  std::println("  ✓ Singleton stardata works correctly");
}

int main() {
  test_entity_manager_basic();
  test_entity_manager_caching();
  test_entity_manager_composite_keys();
  test_entity_manager_create_delete();
  test_entity_manager_read_only_access();
  test_entity_manager_flush_all();
  test_entity_manager_clear_cache();
  test_entity_manager_singleton_stardata();

  std::println("\n✅ All EntityManager tests passed!");
  return 0;
}
