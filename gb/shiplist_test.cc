// SPDX-License-Identifier: Apache-2.0

/// \file shiplist_test.cc
/// \brief Comprehensive unit tests for ShipList iterations (nested, scope, all,
/// all_alive), filters, and const semantics.

import dallib;
import gblib;
import test;
import std;

int main() {
  // Create test context
  TestContext ctx;

  // Create JsonStore for repository operations
  JsonStore store(ctx.db);

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

  // Nested iteration (follows nextship linked list)
  {
    ShipList list(ctx.em, 1);  // Start at ship 1, nested iteration
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
      test::expect_eq(ship.owner(), 1);
    }
    test::expect_eq(count, 3);
    std::println(std::cout, "✓ Test 1 passed: Nested iteration found {} ships",
                 count);
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
    ShipList list(ctx.em, cargo.ships());
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
      test::expect_eq(ship.owner(), 1);
      test::expect_eq(ship.type(), ShipType::OTYPE_PROBE);
    }
    test::expect_eq(count, 2);
    std::println(
        std::cout,
        "✓ Test 1b passed: Multi-level nested iteration found {} inner "
        "ships",
        count);
  }

  // Create GameObj for scope-based tests
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  g.set_player(1);
  g.set_snum(0);
  g.set_pnum(0);
  g.race = ctx.em.peek_race(1);

  // Scope iteration at universe level
  {
    ShipList list(ctx.em, g, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
    }
    // At this point, only ships 1-6 exist (3 original + 3 from Test 1b)
    test::expect_eq(count, 6);
    std::println(std::cout,
                 "✓ Test 2 passed: Scope iteration (UNIV) found {} ships",
                 count);
  }

  // Test 2a: Scope iteration without GameObj defaults to universe scope
  {
    ShipList list(ctx.em, 1, ShipList::IterationType::Scope);
    int mutable_count = 0;
    for (auto handle : list) {
      mutable_count++;
      test::expect_true(handle->alive());
    }
    test::expect_eq(mutable_count, 6);

    const ShipList readonly_list(ctx.em, 1, ShipList::IterationType::Scope);
    int readonly_count = 0;
    for (const Ship* ship : readonly_list) {
      readonly_count++;
      test::expect_true(ship->alive());
    }
    test::expect_eq(readonly_count, 6);

    std::println(
        std::cout,
        "✓ Test 2a passed: Scope iteration without GameObj defaults to UNIV");
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

    auto& registry = get_test_session_registry();
    GameObj g_star(ctx.em, registry);
    g_star.set_player(1);
    g_star.set_level(ScopeLevel::LEVEL_STAR);
    g_star.set_snum(5);
    g_star.race = ctx.em.peek_race(1);

    ShipList list(ctx.em, g_star, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
      test::expect_eq(ship.storbits(), 5);
    }
    test::expect_eq(count, 2);
    std::println(std::cout,
                 "✓ Test 2b passed: Scope iteration (STAR) found {} ships",
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

    auto& registry = get_test_session_registry();
    GameObj g_plan(ctx.em, registry);
    g_plan.set_player(1);
    g_plan.set_level(ScopeLevel::LEVEL_PLAN);
    g_plan.set_snum(10);
    g_plan.set_pnum(3);
    g_plan.race = ctx.em.peek_race(1);

    ShipList list(ctx.em, g_plan, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
      test::expect_eq(ship.storbits(), 10);
      test::expect_eq(ship.pnumorbits(), 3);
    }
    test::expect_eq(count, 1);
    std::println(std::cout,
                 "✓ Test 2c passed: Scope iteration (PLAN) found {} ships",
                 count);
  }

  // Modify ship via handle
  {
    ShipList list(ctx.em, 1, ShipList::IterationType::Nested);
    auto it = list.begin();
    ShipHandle handle = *it;
    Ship& ship = *handle;

    ship.fuel() += 100.0;
    // Handle should auto-save on destruction
  }

  // Verify modification persisted
  {
    const auto* ship = ctx.em.peek_ship(1);
    test::expect_ge(ship->fuel(), 100.0);
    std::println(std::cout,
                 "✓ Test 3 passed: Ship modification persisted via RAII");
  }

  // Test 3b: Multiple ships modified in sequence
  {
    ShipList list(ctx.em, 1, ShipList::IterationType::Nested);
    for (auto handle : list) {
      Ship& ship = *handle;
      ship.fuel() += 50.0;
      ship.destruct() += 10;
    }
    // All modifications should auto-save
  }

  // Verify all modifications persisted
  {
    const auto* ship1 = ctx.em.peek_ship(1);
    const auto* ship2 = ctx.em.peek_ship(2);
    const auto* ship3 = ctx.em.peek_ship(3);
    test::expect_ge(ship1->fuel(), 150.0);  // 100 from test 3 + 50 from test 3b
    test::expect_ge(ship2->fuel(), 50.0);
    test::expect_ge(ship3->fuel(), 50.0);
    test::expect_ge(ship1->destruct(), 10);
    test::expect_ge(ship2->destruct(), 10);
    test::expect_ge(ship3->destruct(), 10);
    std::println(std::cout,
                 "✓ Test 3b passed: Multiple ship modifications persisted");
  }

  // Test 3c: Read-only access via peek()
  {
    ShipList list(ctx.em, 1, ShipList::IterationType::Nested);
    auto it = list.begin();
    ShipHandle handle = *it;

    // Read-only access shouldn't mark dirty
    const Ship& ship_read = handle.peek();
    double initial_fuel = ship_read.fuel();

    // Verify we can read without modification
    test::expect_ge(initial_fuel, 150.0);
    std::println(std::cout, "✓ Test 3c passed: Read-only peek() access works");
  }

  // Ship filtering with ship_matches_filter()
  {
    // Test wildcard filter
    test::expect_true(GB::ship_matches_filter("*", ship1));
    test::expect_true(GB::ship_matches_filter("*", ship2));

    // Test ship type filter (single type)
    // ship1 = OTYPE_FACTORY (index 31) = 'F'
    // ship2 = OTYPE_PROBE (index 29) = ':'
    // ship3 = STYPE_CARGO (index 13) = 'c'
    test::expect_true(GB::ship_matches_filter("F", ship1));   // Factory
    test::expect_false(GB::ship_matches_filter(":", ship1));  // Not a probe
    test::expect_true(GB::ship_matches_filter(":", ship2));   // Probe

    // Test ship type filter (multiple types)
    test::expect_true(GB::ship_matches_filter("F:", ship1));  // Matches factory
    test::expect_true(GB::ship_matches_filter("F:", ship2));  // Matches probe
    test::expect_false(
        GB::ship_matches_filter("cd", ship1));  // Matches neither

    // Test ship number filter
    test::expect_true(GB::ship_matches_filter("#1", ship1));   // ship1 is #1
    test::expect_false(GB::ship_matches_filter("#1", ship2));  // ship2 is #2
    test::expect_true(GB::ship_matches_filter("#2", ship2));   // ship2 is #2

    // Numeric strings WITHOUT '#' are treated as ship type filters
    // They look for ships with type letters matching the digits (e.g., '1',
    // '2', '3') ship1 is type OTYPE_FACTORY = 'F', so "123" won't match
    test::expect_false(GB::ship_matches_filter("123", ship1));

    // Test empty filter
    test::expect_false(GB::ship_matches_filter("", ship1));

    std::println(std::cout,
                 "✓ Test 4 passed: Ship filtering with ship_matches_filter()");
  }

  // Test 4b: parse_ship_selection()
  {
    auto result1 = GB::parse_ship_selection("#123");
    test::expect_true(result1.has_value());
    test::expect_eq(result1.value(), 123);

    auto result2 = GB::parse_ship_selection("456");
    test::expect_true(result2.has_value());
    test::expect_eq(result2.value(), 456);

    auto result3 = GB::parse_ship_selection("f");
    test::expect_false(result3.has_value());

    auto result4 = GB::parse_ship_selection("*");
    test::expect_false(result4.has_value());

    auto result5 = GB::parse_ship_selection("");
    test::expect_false(result5.has_value());

    std::println(std::cout,
                 "✓ Test 4b passed: parse_ship_selection() works correctly");
  }

  // Test 4c: is_ship_number_filter()
  {
    test::expect_true(GB::is_ship_number_filter("#123"));
    test::expect_false(GB::is_ship_number_filter(
        "456"));  // Without '#', it's a ship type filter
    test::expect_false(GB::is_ship_number_filter("f"));
    test::expect_false(GB::is_ship_number_filter("*"));
    test::expect_false(GB::is_ship_number_filter(""));

    std::println(std::cout,
                 "✓ Test 4c passed: is_ship_number_filter() works correctly");
  }

  // Test 4d: Filtering during iteration
  {
    ShipList list(ctx.em, 1, ShipList::IterationType::Nested);
    int factory_count = 0;
    int probe_count = 0;

    for (auto handle : list) {
      const Ship& s = handle.peek();
      if (GB::ship_matches_filter("F", s)) factory_count++;
      if (GB::ship_matches_filter(":", s)) probe_count++;
    }

    test::expect_eq(factory_count, 1);  // ship1 is a factory
    test::expect_eq(probe_count, 1);    // ship2 is a probe

    std::println(std::cout,
                 "✓ Test 4d passed: Filtering during iteration works");
  }

  // Const iteration (read-only, uses peek_ship)
  {
    std::println(std::cout, "\nTest 5: Const iteration (read-only)");

    // Create a const ShipList using const reference
    const ShipList ships_const(ctx.em, 1);

    // Iterate with const iterators - should use peek_ship internally
    int count = 0;
    for (const Ship* ship : ships_const) {
      test::expect_ne(ship, nullptr);
      test::expect_true(ship->alive());
      count++;

      // Read-only operations should work fine
      std::println(std::cout, "  Ship #{}: type={}", ship->number(),
                   static_cast<int>(ship->type()));
    }

    test::expect_eq(count, 3);  // Should see ship1, ship2, ship3

    // Verify ships weren't marked dirty by THIS iteration
    // (they were already modified by Test 3b, so we just check we didn't change
    // them further)
    const auto* check1 = ctx.em.peek_ship(1);
    const auto* check2 = ctx.em.peek_ship(2);
    const auto* check3 = ctx.em.peek_ship(3);
    double fuel1_before = check1->fuel();
    double fuel2_before = check2->fuel();
    double fuel3_before = check3->fuel();

    // Do another const iteration - fuel should remain unchanged
    {
      const ShipList ships_const2(ctx.em, 1);
      for (const Ship* ship : ships_const2) {
        [[maybe_unused]] auto fuel = ship->fuel();
      }
    }

    // Fuel should still be the same (const iteration doesn't mark dirty)
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_before);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_before);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_before);

    std::println(std::cout,
                 "✓ Test 5 passed: Const iteration is truly read-only");
  }

  // Test 5b: Const vs mutable iteration comparison
  {
    std::println(std::cout, "\nTest 5b: Const vs mutable iteration comparison");

    // Get current fuel values before test
    double fuel1_initial = ctx.em.peek_ship(1)->fuel();
    double fuel2_initial = ctx.em.peek_ship(2)->fuel();
    double fuel3_initial = ctx.em.peek_ship(3)->fuel();

    // First, use const iteration - should NOT mark dirty
    {
      const ShipList ships_const(ctx.em, 1);
      for (const Ship* ship : ships_const) {
        // Just reading data
        [[maybe_unused]] auto fuel = ship->fuel();
      }
    }

    // Ships should still have same fuel (not marked dirty)
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_initial);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_initial);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_initial);

    // Now use mutable iteration and actually modify
    {
      ShipList ships_mutable(ctx.em, 1);
      for (auto ship_handle : ships_mutable) {
        Ship& ship = *ship_handle;
        ship.fuel() += 50.0;  // Modify ship
      }
    }  // Ships auto-save here

    // Ships should now have modified fuel
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_initial + 50.0);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_initial + 50.0);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_initial + 50.0);

    std::println(
        std::cout,
        "✓ Test 5b passed: Const iteration doesn't mark dirty, mutable does");
  }

  // Test 5c: Const scope-based iteration
  {
    std::println(std::cout, "\nTest 5c: Const scope-based iteration");

    // Create GameObj for scope-based iteration
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    g.set_player(1);
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(5);

    const ShipList ships(ctx.em, g, ShipList::IterationType::Scope);

    int count = 0;
    for (const Ship* ship : ships) {
      test::expect_ne(ship, nullptr);
      test::expect_eq(ship->storbits(), 5);
      count++;
    }

    test::expect_eq(count, 2);  // ship4 and ship5 are at star 5
    std::println(std::cout,
                 "✓ Test 5c passed: Const scope-based iteration works");
  }

  // IterationType::All - iterates all ships including dead
  {
    std::println(std::cout, "\nTest 6: IterationType::All");

    // Get count of all ships before adding dead ones
    int alive_count = 0;
    {
      ShipList alive_ships(ctx.em, ShipList::IterationType::AllAlive);
      for ([[maybe_unused]] auto handle : alive_ships) {
        alive_count++;
      }
    }
    std::println(std::cout, "  Found {} alive ships before adding dead ship",
                 alive_count);

    // Create a dead ship
    Ship dead_ship{};
    dead_ship.number() = 10;
    dead_ship.owner() = 1;
    dead_ship.alive() = false;  // This ship is dead
    dead_ship.storbits() = 0;
    dead_ship.pnumorbits() = 0;
    dead_ship.type() = ShipType::OTYPE_FACTORY;
    dead_ship.nextship() = 0;
    ships_repo.save(dead_ship);

    // All iteration should include dead ships
    ShipList all_ships(ctx.em, ShipList::IterationType::All);
    int all_count = 0;
    bool found_dead = false;
    for (auto handle : all_ships) {
      all_count++;
      Ship& ship = *handle;
      if (ship.number() == 10 && !ship.alive()) {
        found_dead = true;
      }
    }

    test::expect_eq(all_count,
                    alive_count + 1);  // Should include the dead ship
    test::expect_true(found_dead);
    std::println(
        std::cout,
        "✓ Test 6 passed: All iteration found {} ships (including dead)",
        all_count);
  }

  // IterationType::AllAlive - iterates only alive ships
  {
    std::println(std::cout, "\nTest 7: IterationType::AllAlive");

    ShipList alive_ships(ctx.em, ShipList::IterationType::AllAlive);
    int alive_count = 0;
    bool found_dead = false;
    for (auto handle : alive_ships) {
      alive_count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());  // All ships should be alive
      if (ship.number() == 10) {
        found_dead = true;  // Should not happen
      }
    }

    test::expect_false(
        found_dead);  // Dead ship should not be in AllAlive iteration
    std::println(
        std::cout,
        "✓ Test 7 passed: AllAlive iteration found {} ships (alive only)",
        alive_count);
  }

  // Test 7b: Const All/AllAlive iteration
  {
    std::println(std::cout, "\nTest 7b: Const All/AllAlive iteration");

    // Const All iteration
    const ShipList all_const(ctx.em, ShipList::IterationType::All);
    int all_count = 0;
    for (const Ship* ship : all_const) {
      test::expect_ne(ship, nullptr);
      all_count++;
    }
    test::expect_eq(all_count, 10);  // 9 alive + 1 dead from Test 6
    std::println(std::cout, "  Const All iteration found {} ships", all_count);

    // Const AllAlive iteration
    const ShipList alive_const(ctx.em, ShipList::IterationType::AllAlive);
    int alive_count = 0;
    for (const Ship* ship : alive_const) {
      test::expect_ne(ship, nullptr);
      test::expect_true(ship->alive());
      alive_count++;
    }
    std::println(std::cout, "  Const AllAlive iteration found {} ships",
                 alive_count);
    std::println(std::cout, "  Expected alive_count ({}) == all_count - 1 ({})",
                 alive_count, all_count - 1);
    test::expect_eq(alive_count, all_count - 1);  // One dead ship

    std::println(std::cout,
                 "✓ Test 7b passed: Const All/AllAlive iteration works");
  }

  std::println(std::cout, "\nAll ShipList tests passed!");
  return 0;
}
