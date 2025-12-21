// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  ShipRepository ship_repo(store);

  std::println("Testing gap-finding free ID management...");

  // Test 1: Empty table should return 1
  int id1 = ship_repo.next_available_id();
  assert(id1 == 1);
  std::println("✓ Test 1: Empty table returns ID 1");

  // Test 2: Create ships at 1, 2, 3, verify next is 4
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.name() = "Ship1";
  ship_repo.save(ship1);

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.name() = "Ship2";
  ship_repo.save(ship2);

  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 1;
  ship3.governor() = 0;
  ship3.name() = "Ship3";
  ship_repo.save(ship3);

  int id2 = ship_repo.next_available_id();
  assert(id2 == 4);
  std::println("✓ Test 2: Sequential IDs 1,2,3 -> next is 4");

  // Test 3: Delete ship 2, verify next is 2 (gap reuse)
  ship_repo.delete_ship(2);
  int id3 = ship_repo.next_available_id();
  assert(id3 == 2);
  std::println("✓ Test 3: Delete ship 2 -> next reuses gap at 2");

  // Test 4: Create ship at 2, verify next is 4 again
  Ship ship2b{};
  ship2b.number() = 2;
  ship2b.owner() = 1;
  ship2b.governor() = 0;
  ship2b.name() = "Ship2B";
  ship_repo.save(ship2b);

  int id4 = ship_repo.next_available_id();
  assert(id4 == 4);
  std::println("✓ Test 4: Fill gap at 2 -> next is 4");

  // Test 5: Delete ships 1 and 3, verify next is 1 (smallest gap)
  ship_repo.delete_ship(1);
  ship_repo.delete_ship(3);
  int id5 = ship_repo.next_available_id();
  assert(id5 == 1);
  std::println("✓ Test 5: Multiple gaps -> returns smallest (1)");

  // Test 6: Test commodities work the same way
  std::println("\nTesting commod ID management...");

  CommodRepository commod_repo(store);

  int cid1 = commod_repo.next_available_id();
  assert(cid1 == 1);
  std::println("✓ Test 6: Empty commod table returns ID 1");

  // Create some commods
  Commod c1{};
  c1.id = 1;
  c1.owner = 1;
  c1.governor = 0;
  c1.type = CommodType::RESOURCE;
  c1.amount = 100;
  c1.deliver = true;
  commod_repo.save(c1);

  Commod c2{};
  c2.id = 2;
  c2.owner = 1;
  c2.governor = 0;
  c2.type = CommodType::FUEL;
  c2.amount = 200;
  c2.deliver = true;
  commod_repo.save(c2);

  Commod c4{};
  c4.id = 4;
  c4.owner = 1;
  c4.governor = 0;
  c4.type = CommodType::CRYSTAL;
  c4.amount = 300;
  c4.deliver = true;
  commod_repo.save(c4);

  int cid2 = commod_repo.next_available_id();
  assert(cid2 == 3);
  std::println("✓ Test 7: Commod IDs 1,2,4 -> next is 3 (gap)");

  commod_repo.delete_commod(1);
  int cid3 = commod_repo.next_available_id();
  assert(cid3 == 1);
  std::println("✓ Test 8: Delete commod 1 -> reuses gap at 1");

  std::println("\n✅ All free ID management tests passed!");
  return 0;
}
