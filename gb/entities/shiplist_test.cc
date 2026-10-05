// SPDX-License-Identifier: Apache-2.0

/// \file shiplist_test.cc
/// \brief Comprehensive unit tests for ShipList iterations (scope, all,
/// all_alive, in_star, in_carrier), filters, and const semantics.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import std;

int main() {
  // Create test context with standard universe (stars 1..2, races 1..4)
  TestContext ctx;
  ctx.with_standard_universe();

  // Create JsonStore for repository operations
  JsonStore store(ctx.db);
  StarRepository stars(store);
  stars.save(Star{5, "Star5"});
  stars.save(Star{10, "Star10"});
  PlanetRepository(store).save(Planet{10, 3});

  // Create test ships
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.alive() = true;
  ship1.enter_star_orbit(1);
  ship1.type() = ShipType::OTYPE_FACTORY;
  ship1.max_fuel() = 1000.0;

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.alive() = true;
  ship2.enter_star_orbit(1);
  ship2.type() = ShipType::OTYPE_PROBE;
  ship2.max_fuel() = 1000.0;

  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 1;
  ship3.alive() = true;
  ship3.enter_star_orbit(1);
  ship3.type() = ShipType::STYPE_CARGO;
  ship3.max_fuel() = 1000.0;

  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
  ships_repo.save(ship3);

  // Test 1: Star-scoped iteration via ShipList::in_star
  {
    auto list = ShipList::in_star(ctx.em, starnum_t{1});
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      test::expect_true(ship.alive());
      test::expect_eq(ship.owner(), 1);
    }
    test::expect_eq(count, 3);
    std::println(std::cout,
                 "✓ Test 1 passed: Star-scoped iteration found {} ships",
                 count);
  }

  // Test 1b: Carrier hangar iteration via ShipList::in_carrier
  {
    // Create a cargo ship that contains other ships
    Ship cargo{};
    cargo.number() = 4;  // Use contiguous numbering
    cargo.owner() = 1;
    cargo.alive() = true;
    cargo.enter_star_orbit(1);
    cargo.type() = ShipType::STYPE_CARGO;
    cargo.max_fuel() = 1000.0;

    Ship inner1{};
    inner1.number() = 5;
    inner1.owner() = 1;
    inner1.alive() = true;
    inner1.dock_into_carrier(cargo);
    inner1.type() = ShipType::OTYPE_PROBE;

    Ship inner2{};
    inner2.number() = 6;
    inner2.owner() = 1;
    inner2.alive() = true;
    inner2.dock_into_carrier(cargo);
    inner2.type() = ShipType::OTYPE_PROBE;

    ships_repo.save(cargo);
    ships_repo.save(inner1);
    ships_repo.save(inner2);

    // Iterate over ships contained in cargo via ShipList::in_carrier
    auto list = ShipList::in_carrier(ctx.em, shipnum_t{4});
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
        "✓ Test 1b passed: Carrier hangar iteration found {} inner ships",
        count);
  }

  // Create GameObj for scope-based tests
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  g.set_player(1);
  g.set_snum(0);
  g.set_pnum(0);
  g.race = ctx.em.peek_race(1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

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
    ShipList list(ctx.em, ShipList::IterationType::Scope);
    int mutable_count = 0;
    for (auto handle : list) {
      mutable_count++;
      test::expect_true(handle->alive());
    }
    test::expect_eq(mutable_count, 6);

    const ShipList readonly_list(ctx.em, ShipList::IterationType::Scope);
    int readonly_count = 0;
    for (const Ship& ship : readonly_list) {
      readonly_count++;
      test::expect_true(ship.alive());
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
    star_ship1.enter_star_orbit(5);  // At star 5
    star_ship1.type() = ShipType::OTYPE_FACTORY;

    Ship star_ship2{};
    star_ship2.number() = 8;
    star_ship2.owner() = 1;
    star_ship2.alive() = true;
    star_ship2.enter_star_orbit(5);  // Also at star 5
    star_ship2.type() = ShipType::OTYPE_PROBE;

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
    planet_ship.enter_planet_orbit(10, 3);  // At planet 3 of star 10
    planet_ship.type() = ShipType::STYPE_CARGO;

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
    auto list = ShipList::in_star(ctx.em, starnum_t{1});
    auto it = list.begin();
    ShipHandle handle = *it;
    Ship& ship = *handle;

    ship.add_fuel(100.0);
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
    auto list = ShipList::in_star(ctx.em, starnum_t{1});
    for (auto handle : list) {
      Ship& ship = *handle;
      ship.add_fuel(50.0);
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
    auto list = ShipList::in_star(ctx.em, starnum_t{1});
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
    auto list = ShipList::in_star(ctx.em, starnum_t{1});
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

    // Create a const ShipList using readonly_in_star
    const auto ships_const = ShipList::readonly_in_star(ctx.em, starnum_t{1});

    // Iterate with const iterators - should use peek_ship internally
    int count = 0;
    for (const Ship& ship : ships_const) {
      test::expect_true(ship.alive());
      count++;

      // Read-only operations should work fine
      std::println(std::cout, "  Ship #{}: type={}", ship.number(),
                   static_cast<int>(ship.type()));
    }

    test::expect_eq(count, 4);  // Should see ship1, ship2, ship3, ship4

    // Verify ships weren't marked dirty by THIS iteration
    // (they were already modified by Test 3b, so we just check we didn't change
    // them further)
    const auto* check1 = ctx.em.peek_ship(1);
    const auto* check2 = ctx.em.peek_ship(2);
    const auto* check3 = ctx.em.peek_ship(3);
    const auto* check4 = ctx.em.peek_ship(4);
    double fuel1_before = check1->fuel();
    double fuel2_before = check2->fuel();
    double fuel3_before = check3->fuel();
    double fuel4_before = check4->fuel();

    // Do another const iteration - fuel should remain unchanged
    {
      const auto ships_const2 =
          ShipList::readonly_in_star(ctx.em, starnum_t{1});
      for (const Ship& ship : ships_const2) {
        [[maybe_unused]] auto fuel = ship.fuel();
      }
    }

    // Fuel should still be the same (const iteration doesn't mark dirty)
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_before);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_before);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_before);
    test::expect_eq(ctx.em.peek_ship(4)->fuel(), fuel4_before);

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
    double fuel4_initial = ctx.em.peek_ship(4)->fuel();

    // First, use const iteration - should NOT mark dirty
    {
      const auto ships_const = ShipList::readonly_in_star(ctx.em, starnum_t{1});
      for (const Ship& ship : ships_const) {
        // Just reading data
        [[maybe_unused]] auto fuel = ship.fuel();
      }
    }

    // Ships should still have same fuel (not marked dirty)
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_initial);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_initial);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_initial);
    test::expect_eq(ctx.em.peek_ship(4)->fuel(), fuel4_initial);

    // Now use mutable iteration and actually modify
    {
      auto ships_mutable = ShipList::in_star(ctx.em, starnum_t{1});
      for (auto ship_handle : ships_mutable) {
        Ship& ship = *ship_handle;
        ship.add_fuel(50.0);  // Modify ship
      }
    }  // Ships auto-save here

    // Ships should now have modified fuel
    test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel1_initial + 50.0);
    test::expect_eq(ctx.em.peek_ship(2)->fuel(), fuel2_initial + 50.0);
    test::expect_eq(ctx.em.peek_ship(3)->fuel(), fuel3_initial + 50.0);
    test::expect_eq(ctx.em.peek_ship(4)->fuel(), fuel4_initial + 50.0);

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
    for (const Ship& ship : ships) {
      test::expect_eq(ship.storbits(), 5);
      count++;
    }

    test::expect_eq(count, 2);  // ship7 and ship8 are at star 5
    std::println(std::cout,
                 "✓ Test 5c passed: Const scope-based iteration works");
  }

  // IterationType::All - iterates all ships
  {
    std::println(std::cout, "\nTest 6: IterationType::All");

    int alive_count = 0;
    {
      ShipList alive_ships(ctx.em, ShipList::IterationType::AllAlive);
      for ([[maybe_unused]] auto handle : alive_ships) {
        alive_count++;
      }
    }
    std::println(std::cout, "  Found {} alive ships before adding ship #10",
                 alive_count);

    // Create ship #10
    Ship ship10{};
    ship10.number() = 10;
    ship10.owner() = 1;
    ship10.alive() = true;
    ship10.enter_star_orbit(1);
    ship10.type() = ShipType::OTYPE_FACTORY;
    ships_repo.save(ship10);

    ShipList all_ships(ctx.em, ShipList::IterationType::All);
    int all_count = 0;
    bool found_ship10 = false;
    for (auto handle : all_ships) {
      all_count++;
      Ship& ship = *handle;
      if (ship.number() == 10) {
        found_ship10 = true;
      }
    }

    test::expect_eq(all_count, alive_count + 1);
    test::expect_true(found_ship10);
    std::println(std::cout, "✓ Test 6 passed: All iteration found {} ships",
                 all_count);

    // Destroy ship #10 via kill_ship (hard-deleted from tbl_ship)
    ctx.em.mutate_ship(10, [&](Ship& s) { ctx.em.kill_ship(1, s); });
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
    for (const Ship& ship : all_const) {
      test::expect_true(ship.alive());
      all_count++;
    }
    test::expect_eq(all_count, 9);
    std::println(std::cout, "  Const All iteration found {} ships", all_count);

    // Const AllAlive iteration
    const ShipList alive_const(ctx.em, ShipList::IterationType::AllAlive);
    int alive_count = 0;
    for (const Ship& ship : alive_const) {
      test::expect_true(ship.alive());
      alive_count++;
    }
    std::println(std::cout, "  Const AllAlive iteration found {} ships",
                 alive_count);
    test::expect_eq(alive_count, all_count);

    std::println(std::cout,
                 "✓ Test 7b passed: Const All/AllAlive iteration works");
  }

  // Test 8: Sparse ship IDs (gaps in ID sequence)
  {
    std::println(std::cout, "\nTest 8: Sparse ship IDs iteration");
    TestContext sparse_ctx;
    sparse_ctx.with_standard_universe();
    JsonStore sparse_store(sparse_ctx.db);
    ShipRepository sparse_repo(sparse_store);

    // Create only ship 1 and ship 5 (gaps at 2, 3, 4)
    Ship s1{};
    s1.number() = 1;
    s1.owner() = 1;
    s1.alive() = true;
    s1.enter_star_orbit(1);
    s1.type() = ShipType::OTYPE_PROBE;
    sparse_repo.save(s1);

    Ship s5{};
    s5.number() = 5;
    s5.owner() = 1;
    s5.alive() = true;
    s5.enter_star_orbit(1);
    s5.type() = ShipType::OTYPE_FACTORY;
    sparse_repo.save(s5);

    // Test mutable iteration over sparse IDs
    std::vector<shipnum_t> visited_mutable;
    for (auto handle :
         ShipList(sparse_ctx.em, ShipList::IterationType::AllAlive)) {
      visited_mutable.push_back(handle->number());
    }
    test::expect_eq(visited_mutable.size(), 2);
    test::expect_eq(visited_mutable[0], shipnum_t{1});
    test::expect_eq(visited_mutable[1], shipnum_t{5});

    // Test const iteration over sparse IDs
    std::vector<shipnum_t> visited_const;
    for (const Ship& ship :
         ShipList::readonly(sparse_ctx.em, ShipList::IterationType::All)) {
      visited_const.push_back(ship.number());
    }
    test::expect_eq(visited_const.size(), 2);
    test::expect_eq(visited_const[0], shipnum_t{1});
    test::expect_eq(visited_const[1], shipnum_t{5});

    std::println(
        std::cout,
        "✓ Test 8 passed: Sparse ship IDs iteration visited ships #1 and #5");
  }

  // Test 9: ShipList::in_star excludes ships that break orbit into deep space
  {
    std::println(
        std::cout,
        "\nTest 9: ShipList::in_star excludes deep-space (LEVEL_UNIV) ships");
    ctx.em.mutate_ship(7, [](Ship& s) { s.enter_deep_space(); });

    int star5_count = 0;
    for (const Ship& ship : ShipList::readonly_in_star(ctx.em, starnum_t{5})) {
      test::expect_eq(ship.number(), shipnum_t{8});
      star5_count++;
    }
    test::expect_eq(star5_count, 1);
    std::println(
        std::cout,
        "✓ Test 9 passed: Deep-space ship excluded from ShipList::in_star");
  }

  // Test 10: Mid-loop hard-deletion safety when a ship kills a subsequent ship
  {
    std::println(
        std::cout,
        "\nTest 10: Mid-loop hard-deletion skips destroyed subsequent ships");
    TestContext kill_ctx;
    kill_ctx.with_standard_universe();
    JsonStore kill_store(kill_ctx.db);
    ShipRepository kill_repo(kill_store);

    for (shipnum_t num : {shipnum_t{1}, shipnum_t{2}, shipnum_t{3}}) {
      Ship s{};
      s.number() = num;
      s.owner() = 1;
      s.alive() = true;
      s.enter_star_orbit(1);
      s.type() = ShipType::STYPE_FIGHTER;
      kill_repo.save(s);
    }

    // Mutable iteration: when visiting ship 1, kill ship 2.
    // ShipList iterator must skip deleted ship 2 and advance cleanly to ship 3.
    std::vector<shipnum_t> visited_mutable;
    for (auto handle :
         ShipList(kill_ctx.em, ShipList::IterationType::AllAlive)) {
      visited_mutable.push_back(handle->number());
      if (handle->number() == 1) {
        kill_ctx.em.mutate_ship(
            2, [&](Ship& target) { kill_ctx.em.kill_ship(1, target); });
      }
    }
    test::expect_eq(visited_mutable, (std::vector<shipnum_t>{1, 3}));
    test::expect_throws<EntityNotFoundError>(
        [&]() { kill_ctx.em.peek_ship(2); });

    // Readonly iteration: when visiting ship 1, kill ship 3.
    std::vector<shipnum_t> visited_readonly;
    for (const Ship& ship :
         ShipList::readonly(kill_ctx.em, ShipList::IterationType::AllAlive)) {
      visited_readonly.push_back(ship.number());
      if (ship.number() == 1) {
        kill_ctx.em.mutate_ship(
            3, [&](Ship& target) { kill_ctx.em.kill_ship(1, target); });
      }
    }
    test::expect_eq(visited_readonly, (std::vector<shipnum_t>{1}));
    test::expect_throws<EntityNotFoundError>(
        [&]() { kill_ctx.em.peek_ship(3); });

    std::println(
        std::cout,
        "✓ Test 10 passed: Mid-loop hard-deletion safely skipped killed ships");
  }

  // Test 11: Dead ship in ShipRepository exercises `!alive_only_ ||
  // ship.alive()` for both MutableIterator and ConstIterator, plus LEVEL_SHIP
  // scope and post-increment / cbegin / cend / operator->.
  {
    std::println(std::cout,
                 "\nTest 11: Dead ship alive_only MC/DC and LEVEL_SHIP scope");
    TestContext dead_ctx;
    dead_ctx.with_standard_universe();
    JsonStore dead_store(dead_ctx.db);
    ShipRepository dead_repo(dead_store);

    Ship alive_ship{};
    alive_ship.number() = 1;
    alive_ship.owner() = 1;
    alive_ship.governor() = Race::leader_id;
    alive_ship.alive() = true;
    alive_ship.active() = true;
    alive_ship.enter_star_orbit(1);
    alive_ship.type() = ShipType::STYPE_FIGHTER;
    dead_repo.save(alive_ship);

    Ship dead_ship{};
    dead_ship.number() = 2;
    dead_ship.owner() = 1;
    dead_ship.governor() = Race::leader_id;
    dead_ship.alive() = true;
    dead_ship.active() = true;
    dead_ship.enter_star_orbit(1);
    dead_ship.type() = ShipType::STYPE_FIGHTER;
    dead_repo.save(dead_ship);

    auto dws = dead_ctx.em.create_deferred_write_scope();
    dead_ctx.em.mutate_ship(2, [](Ship& s) { s.alive() = false; });

    ScopeContext ship_scope{
        .player = 1,
        .governor = Race::leader_id,
        .god = false,
        .level = ScopeLevel::LEVEL_SHIP,
        .snum = 1,
        .pnum = 0,
        .shipno = 1,
    };

    // ShipList with ScopeContext: All vs AllAlive vs Scope(LEVEL_SHIP)
    ShipList all_mut(dead_ctx.em, ship_scope, ShipList::IterationType::All);
    std::vector<shipnum_t> all_mut_ids;
    for (auto it = all_mut.begin(); it != all_mut.end(); it++) {
      all_mut_ids.push_back((*it)->number());
    }
    test::expect_eq(all_mut_ids, (std::vector<shipnum_t>{1, 2}));

    const ShipList all_const(dead_ctx.em, ship_scope,
                             ShipList::IterationType::All);
    std::vector<shipnum_t> all_const_ids;
    for (auto it = all_const.cbegin(); it != all_const.cend(); it++) {
      all_const_ids.push_back(it->number());
    }
    test::expect_eq(all_const_ids, (std::vector<shipnum_t>{1, 2}));

    // Explicit vector constructor with alive_only = true skips dead ship #2
    ShipList alive_filtered_mut(dead_ctx.em, std::vector<shipnum_t>{1, 2},
                                /*alive_only=*/true);
    std::vector<shipnum_t> alive_filtered_mut_ids;
    for (auto h : alive_filtered_mut) {
      alive_filtered_mut_ids.push_back(h->number());
    }
    test::expect_eq(alive_filtered_mut_ids, (std::vector<shipnum_t>{1}));

    const ShipList alive_filtered_const(
        dead_ctx.em, std::vector<shipnum_t>{1, 2}, /*alive_only=*/true);
    std::vector<shipnum_t> alive_filtered_const_ids;
    for (const Ship& s : alive_filtered_const) {
      alive_filtered_const_ids.push_back(s.number());
    }
    test::expect_eq(alive_filtered_const_ids, (std::vector<shipnum_t>{1}));

    // ScopeContext at LEVEL_SHIP and AllAlive
    ShipList scope_ship_list(dead_ctx.em, ship_scope,
                             ShipList::IterationType::Scope);
    test::expect_eq(scope_ship_list.size(), 1U);
    ShipList all_alive_ctx_list(dead_ctx.em, ship_scope,
                                ShipList::IterationType::AllAlive);
    test::expect_eq(all_alive_ctx_list.size(), 1U);

    dws.rollback();
    std::println(std::cout, "✓ Test 11 passed");
  }

  // Test 12: resolve_explicit_ship and ScopedCommandableShips
  {
    std::println(std::cout,
                 "\nTest 12: resolve_explicit_ship and ScopedCommandableShips");
    TestContext cmd_ctx;
    cmd_ctx.with_standard_universe();
    JsonStore cmd_store(cmd_ctx.db);
    ShipRepository cmd_repo(cmd_store);

    // Ship 1: owned by player 1, governor 1, active fighter at star 1, planet 1
    Ship s1{};
    s1.number() = 1;
    s1.owner() = 1;
    s1.governor() = Race::leader_id;
    s1.alive() = true;
    s1.active() = true;
    s1.enter_planet_orbit(1, 1);
    s1.type() = ShipType::STYPE_FIGHTER;
    cmd_repo.save(s1);

    // Ship 2: owned by player 2 (foreign!), governor 1, active fighter at star
    // 1, planet 1
    Ship s2{};
    s2.number() = 2;
    s2.owner() = 2;
    s2.governor() = Race::leader_id;
    s2.alive() = true;
    s2.active() = true;
    s2.enter_planet_orbit(1, 1);
    s2.type() = ShipType::STYPE_FIGHTER;
    cmd_repo.save(s2);

    // Ship 3: owned by player 1, governor 1, irradiated/inactive cargo ship at
    // star 1, planet 1
    Ship s3{};
    s3.number() = 3;
    s3.owner() = 1;
    s3.governor() = Race::leader_id;
    s3.alive() = true;
    s3.active() = false;
    s3.enter_planet_orbit(1, 1);
    s3.type() = ShipType::STYPE_CARGO;
    cmd_repo.save(s3);

    // Ship 4: owned by player 1, governor 1, marked dead in memory at star 1,
    // planet 1
    Ship s4{};
    s4.number() = 4;
    s4.owner() = 1;
    s4.governor() = Race::leader_id;
    s4.alive() = true;
    s4.active() = true;
    s4.enter_planet_orbit(1, 1);
    s4.type() = ShipType::STYPE_FIGHTER;
    cmd_repo.save(s4);

    // Ship 5: owned by player 1, governor 1, active fighter at star 1, planet 1
    Ship s5{};
    s5.number() = 5;
    s5.owner() = 1;
    s5.governor() = Race::leader_id;
    s5.alive() = true;
    s5.active() = true;
    s5.enter_planet_orbit(1, 1);
    s5.type() = ShipType::STYPE_FIGHTER;
    cmd_repo.save(s5);

    auto dws = cmd_ctx.em.create_deferred_write_scope();
    cmd_ctx.em.mutate_ship(4, [](Ship& s) { s.alive() = false; });

    ScopeContext p1_leader_star{
        .player = 1,
        .governor = Race::leader_id,
        .god = false,
        .level = ScopeLevel::LEVEL_STAR,
        .snum = 1,
        .pnum = 1,
        .shipno = 0,
    };
    ScopeContext p1_subgov_star = p1_leader_star;
    p1_subgov_star.governor = 2;

    // resolve_explicit_ship checks:
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "*").error(),
        CommandableError::NotOwner);
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#abc").error(),
        CommandableError::NotOwner);
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#0").error(),
        CommandableError::NotOwner);
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#999").error(),
        CommandableError::NotOwner);
    // Foreign ship #2 -> NotOwner
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#2").error(),
        CommandableError::NotOwner);
    // Subordinate governor 2 on governor 0's ship #1 -> NotAuthorizedGovernor
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_subgov_star, "#1").error(),
        CommandableError::NotAuthorizedGovernor);
    // Dead ship #4 -> ShipDead
    test::expect_eq(
        resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#4").error(),
        CommandableError::ShipDead);
    // Irradiated ship #3 with require_active=true -> ShipIrradiated
    test::expect_eq(resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#3",
                                          /*require_active=*/true)
                        .error(),
                    CommandableError::ShipIrradiated);
    // Irradiated ship #3 with require_active=false -> succeeds!
    {
      auto h3 = resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#3",
                                      /*require_active=*/false);
      test::expect_true(h3.has_value());
      test::expect_eq((*h3)->number(), shipnum_t{3});
    }
    // Active owned ship #1 -> succeeds!
    {
      auto h1 = resolve_explicit_ship(cmd_ctx.em, p1_leader_star, "#1");
      test::expect_true(h1.has_value());
      test::expect_eq((*h1)->number(), shipnum_t{1});
    }

    // ScopedCommandableShips:
    // 1. Wildcard "*" with require_active=false yields #1, #3, #5 (silently
    //    skipping foreign #2 and dead #4).
    {
      std::vector<shipnum_t> ids;
      for (auto h : ScopedCommandableShips(cmd_ctx.em, p1_leader_star, "*",
                                           /*require_active=*/false)) {
        ids.push_back(h->number());
      }
      test::expect_eq(ids, (std::vector<shipnum_t>{1, 3, 5}));
    }
    // 2. Wildcard "*" with require_active=true skips irradiated #3 as well.
    {
      std::vector<shipnum_t> ids;
      for (auto h : ScopedCommandableShips(cmd_ctx.em, p1_leader_star, "*",
                                           /*require_active=*/true)) {
        ids.push_back(h->number());
      }
      test::expect_eq(ids, (std::vector<shipnum_t>{1, 5}));
    }
    // 3. Explicit "#2" (foreign ship at same star) yields 0 ships.
    {
      std::vector<shipnum_t> ids;
      for (auto h : ScopedCommandableShips(cmd_ctx.em, p1_leader_star, "#2")) {
        ids.push_back(h->number());
      }
      test::expect_true(ids.empty());
    }
    // 4. Explicit "#1" yields #1 and early-exits on operator++ without scanning
    //    the rest of the list.
    {
      std::vector<shipnum_t> ids;
      ScopedCommandableShips scoped(cmd_ctx.em, p1_leader_star, "#1");
      for (auto it = scoped.begin(); it != scoped.end(); it++) {
        ids.push_back((*it)->number());
      }
      test::expect_eq(ids, (std::vector<shipnum_t>{1}));
    }
    dws.rollback();
    cmd_ctx.em.mutate_ship(
        4, [&](Ship& target) { cmd_ctx.em.kill_ship(1, target); });

    // 5. Exercise LEVEL_UNIV, LEVEL_PLAN, and LEVEL_SHIP scopes + mid-loop
    //    hard-deletion on ScopedCommandableShips.
    for (ScopeLevel lvl : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_PLAN,
                           ScopeLevel::LEVEL_SHIP}) {
      ScopeContext sc = p1_leader_star;
      sc.level = lvl;
      sc.shipno = 1;
      std::vector<shipnum_t> ids;
      for (auto h : ScopedCommandableShips(cmd_ctx.em, sc, "f",
                                           /*require_active=*/true)) {
        ids.push_back(h->number());
      }
      test::expect_eq(ids, (std::vector<shipnum_t>{1, 5}));
    }
    {
      // Kill ship #5 while visiting ship #1 in ScopedCommandableShips
      std::vector<shipnum_t> ids;
      for (auto h :
           ScopedCommandableShips(cmd_ctx.em, p1_leader_star, "f", true)) {
        ids.push_back(h->number());
        if (h->number() == 1) {
          cmd_ctx.em.mutate_ship(
              5, [&](Ship& target) { cmd_ctx.em.kill_ship(1, target); });
        }
      }
      test::expect_eq(ids, (std::vector<shipnum_t>{1}));
    }

    std::println(std::cout, "✓ Test 12 passed");
  }

  std::println(std::cout, "\nAll ShipList tests passed!");
  return 0;
}
