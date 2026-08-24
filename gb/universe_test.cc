// SPDX-License-Identifier: Apache-2.0

/// \file universe_test.cc
/// \brief Unit tests for Universe class accessors, Action Points (AP), Von
/// Neumann (VN) tracking, direct access operators, and EntityManager
/// persistence.

import dallib;
import gblib;
import test;
import std;

void test_universe_wrapper_accessors() {
  std::println(std::cout, "Test: Universe wrapper accessors");

  universe_struct u_data{};
  u_data.id = 1;
  u_data.numstars = 100;
  u_data.ships = 42;

  Universe universe(u_data);

  // Test basic accessors
  test::expect_eq(universe.numstars(), 100);
  test::expect_eq(universe.ships(), 42);

  // Test setters
  universe.set_numstars(150);
  test::expect_eq(universe.numstars(), 150);
  test::expect_eq(u_data.numstars, 150);  // Verify underlying data changed

  universe.set_ships(50);
  test::expect_eq(universe.ships(), 50);
  test::expect_eq(u_data.ships, 50);

  std::println(std::cout, "  ✓ Basic accessors work");
}

void test_universe_AP_methods() {
  std::println(std::cout, "Test: Universe AP (Action Points) methods");

  universe_struct u_data{};
  u_data.id = 1;

  Universe universe(u_data);

  // Set AP for player 1
  universe.set_AP(1, 1000);
  test::expect_eq(universe.get_AP(1), 1000);

  // Deduct AP
  universe.deduct_AP(1, 300);
  test::expect_eq(universe.get_AP(1), 700);

  // Deduct more than available (should clamp to 0)
  universe.deduct_AP(1, 1000);
  test::expect_eq(universe.get_AP(1), 0);

  // Add AP
  universe.add_AP(1, 500);
  test::expect_eq(universe.get_AP(1), 500);

  // Test multiple players
  universe.set_AP(2, 2000);
  universe.set_AP(3, 3000);
  test::expect_eq(universe.get_AP(2), 2000);
  test::expect_eq(universe.get_AP(3), 3000);
  test::expect_eq(universe.get_AP(1), 500);  // Player 1 unaffected

  // Test boundary conditions (invalid player numbers)
  universe.set_AP(0, 999);  // Should be ignored
  test::expect_eq(universe.get_AP(0), 0);

  universe.set_AP(MAXPLAYERS + 1, 999);  // Should be ignored
  test::expect_eq(universe.get_AP(MAXPLAYERS + 1), 0);

  std::println(std::cout, "  ✓ AP methods work correctly");
}

void test_universe_VN_methods() {
  std::println(std::cout, "Test: Universe VN (Von Neumann) tracking methods");

  universe_struct u_data{};
  u_data.id = 1;

  Universe universe(u_data);

  // Test VN hitlist
  universe.set_VN_hitlist(1, 10);
  test::expect_eq(universe.get_VN_hitlist(1), 10);

  universe.increment_VN_hitlist(1);
  test::expect_eq(universe.get_VN_hitlist(1), 11);

  universe.decrement_VN_hitlist(1);
  test::expect_eq(universe.get_VN_hitlist(1), 10);

  // Decrement at 0 should not underflow
  universe.set_VN_hitlist(2, 0);
  universe.decrement_VN_hitlist(2);
  test::expect_eq(universe.get_VN_hitlist(2), 0);

  // Test VN indices (can be negative)
  universe.set_VN_index1(1, -5);
  test::expect_eq(universe.get_VN_index1(1), -5);

  universe.set_VN_index2(1, 100);
  test::expect_eq(universe.get_VN_index2(1), 100);

  universe.set_VN_index1(2, 42);
  universe.set_VN_index2(2, -99);
  test::expect_eq(universe.get_VN_index1(2), 42);
  test::expect_eq(universe.get_VN_index2(2), -99);

  // Test boundary conditions
  test::expect_eq(universe.get_VN_hitlist(0), 0);
  test::expect_eq(universe.get_VN_index1(MAXPLAYERS + 1), 0);

  std::println(std::cout, "  ✓ VN tracking methods work correctly");
}

void test_universe_direct_access() {
  std::println(std::cout, "Test: Universe direct access operators");

  universe_struct u_data{};
  u_data.id = 1;
  u_data.numstars = 50;

  Universe universe(u_data);

  // Test operator->
  test::expect_eq(universe->numstars, 50);
  universe->ships = 123;
  test::expect_eq(universe->ships, 123);

  // Test operator*
  universe_struct& ref = *universe;
  ref.numstars = 75;
  test::expect_eq(universe.numstars(), 75);

  // Test const operator->
  const Universe const_universe(u_data);
  test::expect_eq(const_universe->numstars, 75);

  // Test const operator*
  const universe_struct& const_ref = *const_universe;
  test::expect_eq(const_ref.numstars, 75);

  std::println(std::cout, "  ✓ Direct access operators work correctly");
}

void test_universe_persistence() {
  std::println(std::cout, "Test: Universe persistence with EntityManager");

  Database db(":memory:");
  initialize_schema(db);

  // Create initial universe data in database (singleton with id=1)
  {
    JsonStore store(db);
    UniverseRepository repo(store);

    universe_struct u{};
    u.id = 1;
    u.numstars = 200;
    u.ships = 500;
    u.AP[0] = 1000;  // Player 1
    u.AP[1] = 2000;  // Player 2
    u.VN_hitlist[0] = 5;

    repo.save(u);
  }

  // Now use EntityManager to retrieve and verify
  EntityManager em(db);
  const auto* universe = em.peek_universe();
  test::expect_ne(universe, nullptr);
  test::expect_eq(universe->numstars, 200);
  test::expect_eq(universe->ships, 500);
  test::expect_eq(universe->AP[0], 1000);
  test::expect_eq(universe->AP[1], 2000);
  test::expect_eq(universe->VN_hitlist[0], 5);

  // Modify via EntityManager
  {
    auto universe_handle = em.get_universe();
    auto& universe_mut = *universe_handle;
    universe_mut.numstars = 250;
    universe_mut.ships = 600;
    // Auto-saves when handle goes out of scope
  }

  // Clear cache to force reload from DB
  em.clear_cache();

  // Retrieve and verify modification
  const auto* universe2 = em.peek_universe();
  test::expect_ne(universe2, nullptr);
  test::expect_eq(universe2->numstars, 250);
  test::expect_eq(universe2->ships, 600);
  test::expect_eq(universe2->AP[0], 1000);
  test::expect_eq(universe2->AP[1], 2000);
  test::expect_eq(universe2->VN_hitlist[0], 5);

  std::println(std::cout, "  ✓ Persistence with EntityManager works correctly");
}

int main() {
  test_universe_wrapper_accessors();
  test_universe_AP_methods();
  test_universe_VN_methods();
  test_universe_direct_access();
  test_universe_persistence();

  std::println(std::cout, "\n✅ All Universe tests passed!");
  return 0;
}
