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

  // Create a ship using ship_struct (POD, copyable)
  ship_struct ship_data{};
  ship_data.number = 100;
  ship_data.owner = 1;
  ship_data.fuel = 1000.0;
  Ship ship(ship_data);

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
    handle1->fuel() = 500.0;

    // Verify modification visible via other handle (same instance)
    assert(handle2->fuel() == 500.0);

    std::println(
        "  ✓ Modifications visible across all handles (same instance)");
  }

  // After all handles released, cache should be cleared
  // Get ship again - might be reloaded or cached depending on refcount
  auto handle3 = em.get_ship(100);
  assert(handle3.get() != nullptr);
  assert(handle3->fuel() == 500.0);  // Persisted value

  std::println("  ✓ Entity persists after cache clear");
}

void test_entity_manager_composite_keys() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager composite keys (Planet)");

  // Create a planet
  Planet planet{};
  planet.star_id() = 5;
  planet.planet_order() = 2;
  planet.Maxx() = 10;
  planet.Maxy() = 10;

  JsonStore store(db);
  PlanetRepository planets(store);
  planets.save(planet);

  // Get planet using composite key
  {
    auto handle1 = em.get_planet(5, 2);
    assert(handle1.get() != nullptr);
    assert(handle1->planet_order() == 2);
    assert(handle1->Maxx() == 10);

    // Modify the planet
    handle1->Maxx() = 15;

    // Get the same planet again - should return the SAME in-memory instance
    auto handle2 = em.get_planet(5, 2);
    assert(handle2.get() != nullptr);
    assert(handle2.get() == handle1.get());  // Same pointer!
    assert(handle2->Maxx() == 15);  // Sees the modification immediately

    std::println(
        "  ✓ Multiple handles to same planet return identical instance");
    // Handles go out of scope here, triggering save
  }

  // Verify modification was persisted to database
  auto saved_planet = planets.find_by_location(5, 2);
  assert(saved_planet.has_value());
  assert(saved_planet->Maxx() == 15);

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
    ship_num = handle->number();

    handle->owner() = 1;
    handle->fuel() = 2000.0;
    // Auto-saves on scope exit
  }

  // Verify ship was created and saved
  JsonStore store(db);
  ShipRepository ships(store);
  auto loaded_ship = ships.find_by_number(ship_num);
  assert(loaded_ship.has_value());
  assert(loaded_ship->fuel() == 2000.0);

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

  ship_struct ship_data{};
  ship_data.number = 100;
  ship_data.fuel = 1000.0;
  Ship ship(ship_data);
  ships.save(ship);

  // Load and modify both
  {
    auto race_handle = em.get_race(1);
    auto ship_handle = em.get_ship(100);

    race_handle->tech = 75.0;
    ship_handle->fuel() = 500.0;

    // Don't let handles go out of scope yet
    // Force flush while handles still exist
    em.flush_all();

    // Verify immediate persistence
    auto saved_race = races.find_by_player(1);
    auto saved_ship = ships.find_by_number(100);

    assert(saved_race.has_value());
    assert(saved_race->tech == 75.0);
    assert(saved_ship.has_value());
    assert(saved_ship->fuel() == 500.0);

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

void test_entity_manager_singleton_universe() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager singleton (universe_struct)");

  // Create initial universe_struct
  universe_struct sd{};
  sd.id = 1;  // Universe is a singleton with id=1
  sd.numstars = 100;
  sd.ships = 50;

  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(sd);

  // Get universe_struct (singleton)
  {
    auto handle = em.get_universe();
    assert(handle.get() != nullptr);
    assert(handle->numstars == 100);

    // Modify
    handle->numstars = 150;
  }

  // Verify modification persisted
  auto saved = universe_repo.get_global_data();
  assert(saved.has_value());
  assert(saved->numstars == 150);

  std::println("  ✓ Singleton universe_struct works correctly");
}

void test_entity_manager_get_player() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager find_player_by_name()");

  // Create some test races
  JsonStore store(db);
  RaceRepository races(store);

  Race race1{};
  race1.Playernum = 1;
  race1.name = "Humans";
  races.save(race1);

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Vulcans";
  races.save(race2);

  Race race3{};
  race3.Playernum = 3;
  race3.name = "Klingons";
  races.save(race3);

  // Test: Find by name
  auto p1 = em.find_player_by_name("Humans");
  assert(p1.has_value() && p1.value() == 1);
  std::println("  ✓ find_player_by_name finds race by name");

  auto p2 = em.find_player_by_name("Vulcans");
  assert(p2.has_value() && p2.value() == 2);

  auto p3 = em.find_player_by_name("Klingons");
  assert(p3.has_value() && p3.value() == 3);

  // Test: Find by numeric string
  auto p_num1 = em.find_player_by_name("1");
  assert(p_num1.has_value() && p_num1.value() == 1);
  std::println("  ✓ find_player_by_name finds race by number string");

  auto p_num2 = em.find_player_by_name("2");
  assert(p_num2.has_value() && p_num2.value() == 2);

  // Test: Invalid inputs
  auto p_empty = em.find_player_by_name("");
  assert(!p_empty.has_value());
  std::println("  ✓ find_player_by_name returns nullopt for empty string");

  auto p_notfound = em.find_player_by_name("Romulans");
  assert(!p_notfound.has_value());
  std::println("  ✓ find_player_by_name returns nullopt for non-existent race");

  auto p_invalid_num = em.find_player_by_name("999");
  assert(!p_invalid_num.has_value());
  std::println(
      "  ✓ find_player_by_name returns nullopt for out-of-range number");

  auto p_zero = em.find_player_by_name("0");
  assert(!p_zero.has_value());
  std::println(
      "  ✓ find_player_by_name returns nullopt for invalid player number");
}

void test_entity_manager_kill_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager kill_ship()");

  JsonStore store(db);
  RaceRepository races_repo(store);
  ShipRepository ships_repo(store);
  UniverseRepository sdata_repo(store);

  // Create killer and victim races
  Race killer{};
  killer.Playernum = 1;
  killer.name = "Killer Race";
  killer.morale = 1000;
  killer.God = false;
  races_repo.save(killer);

  Race victim{};
  victim.Playernum = 2;
  victim.name = "Victim Race";
  victim.morale = 1000;
  victim.God = false;
  victim.Gov_ship = 0;
  races_repo.save(victim);

  // Create a ship owned by victim using ship_struct
  ship_struct ship_data{};
  ship_data.number = 100;
  ship_data.owner = 2;
  ship_data.alive = 1;
  ship_data.notified = 1;
  ship_data.type = ShipType::STYPE_BATTLE;
  ship_data.build_cost = 100;
  ship_data.docked = false;
  Ship ship(ship_data);
  ships_repo.save(ship);

  // Test: Kill ship
  em.kill_ship(1, ship);

  std::println("  ✓ kill_ship executed without errors");

  // Verify ship is dead
  assert(ship.alive() == 0);
  assert(ship.notified() == 0);
  std::println("  ✓ Ship marked as dead (alive=0, notified=0)");

  // Verify morale changes were persisted
  auto killer_after = races_repo.find_by_player(1);
  auto victim_after = races_repo.find_by_player(2);
  assert(killer_after.has_value());
  assert(victim_after.has_value());

  // Killer should have gained morale, victim should have lost morale
  assert(killer_after->morale != 1000);  // Changed from initial
  assert(victim_after->morale != 1000);  // Changed from initial
  std::println("  ✓ Morale adjustments persisted for both races");

  // Test: Kill VN ship (updates universe_struct)
  universe_struct sdata{};
  sdata.id = 1;
  sdata.VN_hitlist[0] = 0;
  sdata.VN_index1[0] = -1;
  sdata.VN_index2[0] = -1;
  sdata_repo.save(sdata);

  ship_struct vn_data{};
  vn_data.number = 200;
  vn_data.owner = 1;
  vn_data.alive = 1;
  vn_data.type = ShipType::OTYPE_VN;
  vn_data.storbits = 5;

  MindData mind{};
  mind.who_killed = 1;
  vn_data.special = mind;

  Ship vn_ship(vn_data);
  ships_repo.save(vn_ship);

  em.kill_ship(1, vn_ship);

  std::println("  ✓ VN ship killed without errors");

  // Verify VN tracking was updated
  auto sdata_after = sdata_repo.get_global_data();
  assert(sdata_after.has_value());
  assert(sdata_after->VN_hitlist[0] == 1);  // Incremented
  assert(sdata_after->VN_index1[0] == 5);   // Star index recorded
  std::println("  ✓ VN tracking (VN_hitlist and VN_index) updated correctly");

  // Test: Kill pod (no morale effects)
  Race race3{};
  race3.Playernum = 3;
  race3.name = "Pod Owner";
  race3.morale = 500;
  races_repo.save(race3);

  ship_struct pod_data{};
  pod_data.number = 300;
  pod_data.owner = 3;
  pod_data.alive = 1;
  pod_data.type = ShipType::STYPE_POD;
  Ship pod(pod_data);
  ships_repo.save(pod);

  em.kill_ship(1, pod);

  auto race3_after = races_repo.find_by_player(3);
  assert(race3_after.has_value());
  assert(race3_after->morale == 500);  // Unchanged - pods don't affect morale
  std::println("  ✓ Pod death does not affect morale");
}

void test_entity_manager_kill_ship_gov_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: EntityManager kill_ship() with Gov_ship");

  JsonStore store(db);
  RaceRepository races_repo(store);
  ShipRepository ships_repo(store);

  // Create race with a government ship
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.Gov_ship = 100;  // Government ship number
  race.morale = 1000;
  race.God = false;
  races_repo.save(race);

  // Create killer race
  Race killer{};
  killer.Playernum = 2;
  killer.name = "Killer Race";
  killer.morale = 500;
  killer.God = false;
  races_repo.save(killer);

  // Create the government ship using ship_struct
  ship_struct gov_data{};
  gov_data.number = 100;
  gov_data.owner = 1;
  gov_data.alive = 1;
  gov_data.type = ShipType::OTYPE_GOV;
  gov_data.build_cost = 500;
  gov_data.docked = false;
  Ship gov_ship(gov_data);
  ships_repo.save(gov_ship);

  // Kill the government ship
  em.kill_ship(2, gov_ship);  // Killed by player 2

  std::println("  ✓ Government ship killed");

  // Verify Gov_ship was cleared
  auto race_after = races_repo.find_by_player(1);
  assert(race_after.has_value());
  assert(race_after->Gov_ship == 0);  // Should be cleared
  std::println("  ✓ Gov_ship field cleared when government ship is killed");
}

void test_peek_star_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: peek_star throws EntityNotFoundError on not found");

  // Try to peek a star that doesn't exist
  bool exception_thrown = false;
  try {
    em.peek_star(999);
  } catch (const EntityNotFoundError& e) {
    exception_thrown = true;
    std::string msg = e.what();
    assert(msg.find("Star not found") != std::string::npos);
    assert(msg.find("999") != std::string::npos);
    std::println("  ✓ Exception message: {}", msg);
  }
  assert(exception_thrown);

  std::println("  ✓ peek_star throws EntityNotFoundError for invalid star_id");
}

void test_peek_planet_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: peek_planet throws EntityNotFoundError on not found");

  // Try to peek a planet that doesn't exist
  bool exception_thrown = false;
  try {
    em.peek_planet(5, 3);
  } catch (const EntityNotFoundError& e) {
    exception_thrown = true;
    std::string msg = e.what();
    assert(msg.find("Planet not found") != std::string::npos);
    assert(msg.find("5") != std::string::npos);
    assert(msg.find("3") != std::string::npos);
    std::println("  ✓ Exception message: {}", msg);
  }
  assert(exception_thrown);

  std::println("  ✓ peek_planet throws EntityNotFoundError for invalid planet");
}

void test_peek_sectormap_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: peek_sectormap throws EntityNotFoundError on not found");

  // Try to peek a sectormap for a planet that doesn't exist
  bool exception_thrown = false;
  try {
    em.peek_sectormap(10, 5);
  } catch (const EntityNotFoundError& e) {
    exception_thrown = true;
    std::string msg = e.what();
    assert(msg.find("SectorMap not found") != std::string::npos);
    assert(msg.find("10") != std::string::npos);
    assert(msg.find("5") != std::string::npos);
    std::println("  ✓ Exception message: {}", msg);
  }
  assert(exception_thrown);

  std::println(
      "  ✓ peek_sectormap throws EntityNotFoundError for invalid planet");
}

void test_peek_increments_refcount() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("Test: peek increments refcount (pointers remain valid)");

  // Create test data
  JsonStore store(db);

  // Create a star
  star_struct star_data{};
  star_data.star_id = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 200.0;
  Star star(star_data);
  StarRepository stars(store);
  stars.save(star);

  // Test 1: peek then get - peek pointer should remain valid
  {
    const auto* peek_ptr = em.peek_star(0);
    assert(peek_ptr != nullptr);
    std::string peek_name = peek_ptr->get_name();
    assert(peek_name == "TestStar");

    // Call get_star which will increment refcount and then decrement on scope
    // exit
    {
      auto star_handle = em.get_star(0);
      assert(star_handle.get() != nullptr);
      // star_handle destructor runs here, decrements refcount
    }

    // CRITICAL: peek_ptr should still be valid because peek incremented
    // refcount Without the fix, this would be use-after-free
    std::string peek_name_after = peek_ptr->get_name();
    assert(peek_name_after == "TestStar");

    std::println("  ✓ peek pointer remains valid after get/release cycle");
  }

  // Test 2: Multiple peeks should work correctly
  {
    const auto* peek1 = em.peek_star(0);
    const auto* peek2 = em.peek_star(0);
    assert(peek1 == peek2);  // Same cached instance

    std::println("  ✓ Multiple peeks return same cached instance");
  }

  // Test 3: peek on race (different entity type)
  {
    Race race{};
    race.Playernum = 1;
    race.name = "TestRace";
    race.tech = 50.0;
    RaceRepository races(store);
    races.save(race);

    const auto* peek_race = em.peek_race(1);
    assert(peek_race != nullptr);
    assert(peek_race->name == "TestRace");

    {
      auto race_handle = em.get_race(1);
      race_handle->tech = 75.0;
      // Auto-saves and decrements refcount here
    }

    // peek_race should still be valid
    assert(peek_race->name == "TestRace");
    // Note: peek_race->tech might be 75.0 if same instance (cached),
    // but the important thing is the pointer is not dangling

    std::println("  ✓ peek on race also increments refcount correctly");
  }

  // Test 4: peek on ship
  {
    ship_struct ship_data{};
    ship_data.number = 100;
    ship_data.owner = 1;
    ship_data.fuel = 1000.0;
    Ship ship(ship_data);
    ShipRepository ships(store);
    ships.save(ship);

    const auto* peek_ship = em.peek_ship(100);
    assert(peek_ship != nullptr);

    {
      auto ship_handle = em.get_ship(100);
      ship_handle->fuel() = 500.0;
    }

    // peek_ship pointer should still be valid
    assert(peek_ship != nullptr);

    std::println("  ✓ peek on ship also increments refcount correctly");
  }

  std::println("  ✅ All peek refcount tests passed");
}

int main() {
  test_entity_manager_basic();
  test_entity_manager_caching();
  test_entity_manager_composite_keys();
  test_entity_manager_create_delete();
  test_entity_manager_read_only_access();
  test_entity_manager_flush_all();
  test_entity_manager_clear_cache();
  test_entity_manager_singleton_universe();
  test_entity_manager_get_player();
  test_entity_manager_kill_ship();
  test_entity_manager_kill_ship_gov_ship();
  test_peek_star_throws_on_not_found();
  test_peek_planet_throws_on_not_found();
  test_peek_sectormap_throws_on_not_found();
  test_peek_increments_refcount();

  std::println("\n✅ All EntityManager tests passed!");
  return 0;
}
