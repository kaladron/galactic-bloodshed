// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create EntityManager for accessing entities
  EntityManager em(db);

  // Create JsonStore for repository operations
  JsonStore store(db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].money = 1000;

  RaceRepository races(store);
  races.save(race);

  // Create test ships
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.alive() = true;
  ship1.storbits() = 0;
  ship1.pnumorbits() = 0;
  ship1.type() = ShipType::OTYPE_FACTORY;
  ship1.nextship() = 2;  // Linked list

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.alive() = true;
  ship2.storbits() = 0;
  ship2.pnumorbits() = 0;
  ship2.type() = ShipType::OTYPE_PROBE;
  ship2.nextship() = 3;

  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 1;
  ship3.alive() = true;
  ship3.storbits() = 0;
  ship3.pnumorbits() = 0;
  ship3.type() = ShipType::STYPE_CARGO;
  ship3.nextship() = 0;  // End of list

  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
  ships_repo.save(ship3);

  // Test 1: Nested iteration (follows nextship linked list)
  {
    ShipList list(em, 1);  // Start at ship 1, nested iteration
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive());
      assert(ship.owner() == 1);
    }
    assert(count == 3);
    std::println("✓ Test 1 passed: Nested iteration found {} ships", count);
  }

  // Test 1b: Multi-level nested iteration (ships within ships)
  {
    // Create a cargo ship that contains other ships
    Ship cargo{};
    cargo.number() = 4;  // Use contiguous numbering
    cargo.owner() = 1;
    cargo.alive() = true;
    cargo.storbits() = 0;
    cargo.pnumorbits() = 0;
    cargo.type() = ShipType::STYPE_CARGO;
    cargo.ships() = 5;  // Contains ship 5
    cargo.nextship() = 0;

    Ship inner1{};
    inner1.number() = 5;
    inner1.owner() = 1;
    inner1.alive() = true;
    inner1.storbits() = 0;
    inner1.pnumorbits() = 0;
    inner1.type() = ShipType::OTYPE_PROBE;
    inner1.ships() = 0;
    inner1.nextship() = 6;  // Linked to ship 6

    Ship inner2{};
    inner2.number() = 6;
    inner2.owner() = 1;
    inner2.alive() = true;
    inner2.storbits() = 0;
    inner2.pnumorbits() = 0;
    inner2.type() = ShipType::OTYPE_PROBE;
    inner2.ships() = 0;
    inner2.nextship() = 0;

    ships_repo.save(cargo);
    ships_repo.save(inner1);
    ships_repo.save(inner2);

    // Iterate over ships contained in cargo (ship 5's nextship chain)
    ShipList list(em, cargo.ships());
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive());
      assert(ship.owner() == 1);
      assert(ship.type() == ShipType::OTYPE_PROBE);
    }
    assert(count == 2);
    std::println(
        "✓ Test 1b passed: Multi-level nested iteration found {} inner "
        "ships",
        count);
  }

  // Create GameObj for scope-based tests
  GameObj g(em);
  g.player = 1;
  g.snum = 0;
  g.pnum = 0;
  g.race = em.peek_race(1);

  // Test 2: Scope iteration at universe level
  {
    ShipList list(em, g, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive());
    }
    // At this point, only ships 1-6 exist (3 original + 3 from Test 1b)
    assert(count == 6);
    std::println("✓ Test 2 passed: Scope iteration (UNIV) found {} ships",
                 count);
  }

  // Test 2b: Scope iteration at star level
  {
    // Create ships at specific star
    Ship star_ship1{};
    star_ship1.number() = 7;
    star_ship1.owner() = 1;
    star_ship1.alive() = true;
    star_ship1.storbits() = 5;  // At star 5
    star_ship1.pnumorbits() = -1;
    star_ship1.type() = ShipType::OTYPE_FACTORY;
    star_ship1.nextship() = 0;

    Ship star_ship2{};
    star_ship2.number() = 8;
    star_ship2.owner() = 1;
    star_ship2.alive() = true;
    star_ship2.storbits() = 5;  // Also at star 5
    star_ship2.pnumorbits() = -1;
    star_ship2.type() = ShipType::OTYPE_PROBE;
    star_ship2.nextship() = 0;

    ships_repo.save(star_ship1);
    ships_repo.save(star_ship2);

    GameObj g_star(em);
    g_star.player = 1;
    g_star.level = ScopeLevel::LEVEL_STAR;
    g_star.snum = 5;
    g_star.race = em.peek_race(1);

    ShipList list(em, g_star, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive());
      assert(ship.storbits() == 5);
    }
    assert(count == 2);
    std::println("✓ Test 2b passed: Scope iteration (STAR) found {} ships",
                 count);
  }

  // Test 2c: Scope iteration at planet level
  {
    // Create ships at specific planet
    Ship planet_ship{};
    planet_ship.number() = 9;
    planet_ship.owner() = 1;
    planet_ship.alive() = true;
    planet_ship.storbits() = 10;
    planet_ship.pnumorbits() = 3;  // At planet 3 of star 10
    planet_ship.type() = ShipType::STYPE_CARGO;
    planet_ship.nextship() = 0;

    ships_repo.save(planet_ship);

    GameObj g_plan(em);
    g_plan.player = 1;
    g_plan.level = ScopeLevel::LEVEL_PLAN;
    g_plan.snum = 10;
    g_plan.pnum = 3;
    g_plan.race = em.peek_race(1);

    ShipList list(em, g_plan, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive());
      assert(ship.storbits() == 10);
      assert(ship.pnumorbits() == 3);
    }
    assert(count == 1);
    std::println("✓ Test 2c passed: Scope iteration (PLAN) found {} ships",
                 count);
  }

  // Test 3: Modify ship via handle
  {
    ShipList list(em, 1, ShipList::IterationType::Nested);
    auto it = list.begin();
    ShipHandle handle = *it;
    Ship& ship = *handle;

    ship.fuel() += 100.0;
    // Handle should auto-save on destruction
  }

  // Verify modification persisted
  {
    const auto* ship = em.peek_ship(1);
    assert(ship->fuel() >= 100.0);
    std::println("✓ Test 3 passed: Ship modification persisted via RAII");
  }

  // Test 3b: Multiple ships modified in sequence
  {
    ShipList list(em, 1, ShipList::IterationType::Nested);
    for (auto handle : list) {
      Ship& ship = *handle;
      ship.fuel() += 50.0;
      ship.destruct() += 10;
    }
    // All modifications should auto-save
  }

  // Verify all modifications persisted
  {
    const auto* ship1 = em.peek_ship(1);
    const auto* ship2 = em.peek_ship(2);
    const auto* ship3 = em.peek_ship(3);
    assert(ship1->fuel() >= 150.0);  // 100 from test 3 + 50 from test 3b
    assert(ship2->fuel() >= 50.0);
    assert(ship3->fuel() >= 50.0);
    assert(ship1->destruct() >= 10);
    assert(ship2->destruct() >= 10);
    assert(ship3->destruct() >= 10);
    std::println("✓ Test 3b passed: Multiple ship modifications persisted");
  }

  // Test 3c: Read-only access via peek()
  {
    ShipList list(em, 1, ShipList::IterationType::Nested);
    auto it = list.begin();
    ShipHandle handle = *it;

    // Read-only access shouldn't mark dirty
    const Ship& ship_read = handle.peek();
    double initial_fuel = ship_read.fuel();

    // Verify we can read without modification
    assert(initial_fuel >= 150.0);
    std::println("✓ Test 3c passed: Read-only peek() access works");
  }

  // Test 4: Ship filtering with ship_matches_filter()
  {
    // Test wildcard filter
    assert(GB::ship_matches_filter("*", ship1) == true);
    assert(GB::ship_matches_filter("*", ship2) == true);

    // Test ship type filter (single type)
    // ship1 = OTYPE_FACTORY (index 31) = 'F'
    // ship2 = OTYPE_PROBE (index 29) = ':'
    // ship3 = STYPE_CARGO (index 13) = 'c'
    assert(GB::ship_matches_filter("F", ship1) == true);   // Factory
    assert(GB::ship_matches_filter(":", ship1) == false);  // Not a probe
    assert(GB::ship_matches_filter(":", ship2) == true);   // Probe

    // Test ship type filter (multiple types)
    assert(GB::ship_matches_filter("F:", ship1) == true);   // Matches factory
    assert(GB::ship_matches_filter("F:", ship2) == true);   // Matches probe
    assert(GB::ship_matches_filter("cd", ship1) == false);  // Matches neither

    // Test ship number filter
    assert(GB::ship_matches_filter("#1", ship1) == true);   // ship1 is #1
    assert(GB::ship_matches_filter("#1", ship2) == false);  // ship2 is #2
    assert(GB::ship_matches_filter("#2", ship2) == true);   // ship2 is #2

    // Numeric strings WITHOUT '#' are treated as ship type filters
    // They look for ships with type letters matching the digits (e.g., '1',
    // '2', '3') ship1 is type OTYPE_FACTORY = 'F', so "123" won't match
    assert(GB::ship_matches_filter("123", ship1) == false);

    // Test empty filter
    assert(GB::ship_matches_filter("", ship1) == false);

    std::println("✓ Test 4 passed: Ship filtering with ship_matches_filter()");
  }

  // Test 4b: parse_ship_selection()
  {
    auto result1 = GB::parse_ship_selection("#123");
    assert(result1.has_value());
    assert(result1.value() == 123);

    auto result2 = GB::parse_ship_selection("456");
    assert(result2.has_value());
    assert(result2.value() == 456);

    auto result3 = GB::parse_ship_selection("f");
    assert(!result3.has_value());

    auto result4 = GB::parse_ship_selection("*");
    assert(!result4.has_value());

    auto result5 = GB::parse_ship_selection("");
    assert(!result5.has_value());

    std::println("✓ Test 4b passed: parse_ship_selection() works correctly");
  }

  // Test 4c: is_ship_number_filter()
  {
    assert(GB::is_ship_number_filter("#123") == true);
    assert(GB::is_ship_number_filter("456") ==
           false);  // Without '#', it's a ship type filter
    assert(GB::is_ship_number_filter("f") == false);
    assert(GB::is_ship_number_filter("*") == false);
    assert(GB::is_ship_number_filter("") == false);

    std::println("✓ Test 4c passed: is_ship_number_filter() works correctly");
  }

  // Test 4d: Filtering during iteration
  {
    ShipList list(em, 1, ShipList::IterationType::Nested);
    int factory_count = 0;
    int probe_count = 0;

    for (auto handle : list) {
      const Ship& s = handle.peek();
      if (GB::ship_matches_filter("F", s)) factory_count++;
      if (GB::ship_matches_filter(":", s)) probe_count++;
    }

    assert(factory_count == 1);  // ship1 is a factory
    assert(probe_count == 1);    // ship2 is a probe

    std::println("✓ Test 4d passed: Filtering during iteration works");
  }

  // Test 5: Const iteration (read-only, uses peek_ship)
  {
    std::println("\nTest 5: Const iteration (read-only)");

    // Create a const ShipList using const reference
    const ShipList ships_const(em, 1);

    // Iterate with const iterators - should use peek_ship internally
    int count = 0;
    for (const Ship* ship : ships_const) {
      assert(ship != nullptr);
      assert(ship->alive());
      count++;

      // Read-only operations should work fine
      std::println("  Ship #{}: type={}", ship->number(),
                   static_cast<int>(ship->type()));
    }

    assert(count == 3);  // Should see ship1, ship2, ship3

    // Verify ships weren't marked dirty by THIS iteration
    // (they were already modified by Test 3b, so we just check we didn't change
    // them further)
    const auto* check1 = em.peek_ship(1);
    const auto* check2 = em.peek_ship(2);
    const auto* check3 = em.peek_ship(3);
    double fuel1_before = check1->fuel();
    double fuel2_before = check2->fuel();
    double fuel3_before = check3->fuel();

    // Do another const iteration - fuel should remain unchanged
    {
      const ShipList ships_const2(em, 1);
      for (const Ship* ship : ships_const2) {
        [[maybe_unused]] auto fuel = ship->fuel();
      }
    }

    // Fuel should still be the same (const iteration doesn't mark dirty)
    assert(em.peek_ship(1)->fuel() == fuel1_before);
    assert(em.peek_ship(2)->fuel() == fuel2_before);
    assert(em.peek_ship(3)->fuel() == fuel3_before);

    std::println("✓ Test 5 passed: Const iteration is truly read-only");
  }

  // Test 5b: Const vs mutable iteration comparison
  {
    std::println("\nTest 5b: Const vs mutable iteration comparison");

    // Get current fuel values before test
    double fuel1_initial = em.peek_ship(1)->fuel();
    double fuel2_initial = em.peek_ship(2)->fuel();
    double fuel3_initial = em.peek_ship(3)->fuel();

    // First, use const iteration - should NOT mark dirty
    {
      const ShipList ships_const(em, 1);
      for (const Ship* ship : ships_const) {
        // Just reading data
        [[maybe_unused]] auto fuel = ship->fuel();
      }
    }

    // Ships should still have same fuel (not marked dirty)
    assert(em.peek_ship(1)->fuel() == fuel1_initial);
    assert(em.peek_ship(2)->fuel() == fuel2_initial);
    assert(em.peek_ship(3)->fuel() == fuel3_initial);

    // Now use mutable iteration and actually modify
    {
      ShipList ships_mutable(em, 1);
      for (auto ship_handle : ships_mutable) {
        Ship& ship = *ship_handle;
        ship.fuel() += 50.0;  // Modify ship
      }
    }  // Ships auto-save here

    // Ships should now have modified fuel
    assert(em.peek_ship(1)->fuel() == fuel1_initial + 50.0);
    assert(em.peek_ship(2)->fuel() == fuel2_initial + 50.0);
    assert(em.peek_ship(3)->fuel() == fuel3_initial + 50.0);

    std::println(
        "✓ Test 5b passed: Const iteration doesn't mark dirty, mutable does");
  }

  // Test 5c: Const scope-based iteration
  {
    std::println("\nTest 5c: Const scope-based iteration");

    // Create GameObj for scope-based iteration
    GameObj g(em);
    g.player = 1;
    g.level = ScopeLevel::LEVEL_STAR;
    g.snum = 5;

    const ShipList ships(em, g, ShipList::IterationType::Scope);

    int count = 0;
    for (const Ship* ship : ships) {
      assert(ship != nullptr);
      assert(ship->storbits() == 5);
      count++;
    }

    assert(count == 2);  // ship4 and ship5 are at star 5
    std::println("✓ Test 5c passed: Const scope-based iteration works");
  }

  std::println("\nAll ShipList tests passed!");
  return 0;
}
