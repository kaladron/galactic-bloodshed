// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

void test_universe_wrapper_accessors() {
  std::println("Test: Universe wrapper accessors");

  universe_struct u_data{};
  u_data.id = 1;
  u_data.numstars = 100;
  u_data.ships = 42;

  Universe universe(u_data);

  // Test basic accessors
  assert(universe.numstars() == 100);
  assert(universe.ships() == 42);

  // Test setters
  universe.set_numstars(150);
  assert(universe.numstars() == 150);
  assert(u_data.numstars == 150);  // Verify underlying data changed

  universe.set_ships(50);
  assert(universe.ships() == 50);
  assert(u_data.ships == 50);

  std::println("  ✓ Basic accessors work");
}

void test_universe_AP_methods() {
  std::println("Test: Universe AP (Action Points) methods");

  universe_struct u_data{};
  u_data.id = 1;

  Universe universe(u_data);

  // Set AP for player 1
  universe.set_AP(1, 1000);
  assert(universe.get_AP(1) == 1000);

  // Deduct AP
  universe.deduct_AP(1, 300);
  assert(universe.get_AP(1) == 700);

  // Deduct more than available (should clamp to 0)
  universe.deduct_AP(1, 1000);
  assert(universe.get_AP(1) == 0);

  // Add AP
  universe.add_AP(1, 500);
  assert(universe.get_AP(1) == 500);

  // Test multiple players
  universe.set_AP(2, 2000);
  universe.set_AP(3, 3000);
  assert(universe.get_AP(2) == 2000);
  assert(universe.get_AP(3) == 3000);
  assert(universe.get_AP(1) == 500);  // Player 1 unaffected

  // Test boundary conditions (invalid player numbers)
  universe.set_AP(0, 999);  // Should be ignored
  assert(universe.get_AP(0) == 0);

  universe.set_AP(MAXPLAYERS + 1, 999);  // Should be ignored
  assert(universe.get_AP(MAXPLAYERS + 1) == 0);

  std::println("  ✓ AP methods work correctly");
}

void test_universe_VN_methods() {
  std::println("Test: Universe VN (Von Neumann) tracking methods");

  universe_struct u_data{};
  u_data.id = 1;

  Universe universe(u_data);

  // Test VN hitlist
  universe.set_VN_hitlist(1, 10);
  assert(universe.get_VN_hitlist(1) == 10);

  universe.increment_VN_hitlist(1);
  assert(universe.get_VN_hitlist(1) == 11);

  universe.decrement_VN_hitlist(1);
  assert(universe.get_VN_hitlist(1) == 10);

  // Decrement at 0 should not underflow
  universe.set_VN_hitlist(2, 0);
  universe.decrement_VN_hitlist(2);
  assert(universe.get_VN_hitlist(2) == 0);

  // Test VN indices (can be negative)
  universe.set_VN_index1(1, -5);
  assert(universe.get_VN_index1(1) == -5);

  universe.set_VN_index2(1, 100);
  assert(universe.get_VN_index2(1) == 100);

  universe.set_VN_index1(2, 42);
  universe.set_VN_index2(2, -99);
  assert(universe.get_VN_index1(2) == 42);
  assert(universe.get_VN_index2(2) == -99);

  // Test boundary conditions
  assert(universe.get_VN_hitlist(0) == 0);
  assert(universe.get_VN_index1(MAXPLAYERS + 1) == 0);

  std::println("  ✓ VN tracking methods work correctly");
}

void test_universe_direct_access() {
  std::println("Test: Universe direct access operators");

  universe_struct u_data{};
  u_data.id = 1;
  u_data.numstars = 50;

  Universe universe(u_data);

  // Test operator->
  assert(universe->numstars == 50);
  universe->ships = 123;
  assert(universe->ships == 123);

  // Test operator*
  universe_struct& ref = *universe;
  ref.numstars = 75;
  assert(universe.numstars() == 75);

  // Test const operator->
  const Universe const_universe(u_data);
  assert(const_universe->numstars == 75);

  // Test const operator*
  const universe_struct& const_ref = *const_universe;
  assert(const_ref.numstars == 75);

  std::println("  ✓ Direct access operators work correctly");
}

void test_universe_persistence() {
  std::println("Test: Universe persistence with EntityManager");

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
  assert(universe);
  assert(universe->numstars == 200);
  assert(universe->ships == 500);
  assert(universe->AP[0] == 1000);
  assert(universe->AP[1] == 2000);
  assert(universe->VN_hitlist[0] == 5);

  // Modify via EntityManager
  {
    auto universe_handle = em.get_universe();
    auto& universe = *universe_handle;
    universe.numstars = 250;
    universe.ships = 600;
    // Auto-saves when handle goes out of scope
  }

  // Clear cache to force reload from DB
  em.clear_cache();

  // Retrieve and verify modification
  const auto* universe2 = em.peek_universe();
  assert(universe2);
  assert(universe2->numstars == 250);
  assert(universe2->ships == 600);
  assert(universe2->AP[0] == 1000);
  assert(universe2->AP[1] == 2000);
  assert(universe2->VN_hitlist[0] == 5);

  std::println("  ✓ Persistence with EntityManager works correctly");
}

int main() {
  test_universe_wrapper_accessors();
  test_universe_AP_methods();
  test_universe_VN_methods();
  test_universe_direct_access();
  test_universe_persistence();

  std::println("\n✅ All Universe tests passed!");
  return 0;
}
