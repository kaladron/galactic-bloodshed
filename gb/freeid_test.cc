// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  std::println("Testing gap-finding free ID management...");

  // Test 1: Empty table should return 1
  int id1 = getdeadship();
  assert(id1 == 1);
  std::println("✓ Test 1: Empty table returns ID 1");

  // Test 2: Create ships at 1, 2, 3, verify next is 4
  Ship ship1{};
  ship1.number = 1;
  ship1.owner = 1;
  ship1.governor = 0;
  ship1.name = "Ship1";
  putship(ship1);

  Ship ship2{};
  ship2.number = 2;
  ship2.owner = 1;
  ship2.governor = 0;
  ship2.name = "Ship2";
  putship(ship2);

  Ship ship3{};
  ship3.number = 3;
  ship3.owner = 1;
  ship3.governor = 0;
  ship3.name = "Ship3";
  putship(ship3);

  int id2 = getdeadship();
  assert(id2 == 4);
  std::println("✓ Test 2: Sequential IDs 1,2,3 -> next is 4");

  // Test 3: Delete ship 2, verify next is 2 (gap reuse)
  makeshipdead(2);
  int id3 = getdeadship();
  assert(id3 == 2);
  std::println("✓ Test 3: Delete ship 2 -> next reuses gap at 2");

  // Test 4: Create ship at 2, verify next is 4 again
  Ship ship2b{};
  ship2b.number = 2;
  ship2b.owner = 1;
  ship2b.governor = 0;
  ship2b.name = "Ship2B";
  putship(ship2b);

  int id4 = getdeadship();
  assert(id4 == 4);
  std::println("✓ Test 4: Fill gap at 2 -> next is 4");

  // Test 5: Delete ships 1 and 3, verify next is 1 (smallest gap)
  makeshipdead(1);
  makeshipdead(3);
  int id5 = getdeadship();
  assert(id5 == 1);
  std::println("✓ Test 5: Multiple gaps -> returns smallest (1)");

  // Test 6: Test commodities work the same way
  std::println("\nTesting commod ID management...");
  
  int cid1 = getdeadcommod();
  assert(cid1 == 1);
  std::println("✓ Test 6: Empty commod table returns ID 1");

  // Create some commods
  Commod c1{};
  c1.owner = 1;
  c1.governor = 0;
  c1.type = CommodType::RESOURCE;
  c1.amount = 100;
  c1.deliver = true;
  c1.bid = 0;
  c1.bidder = 0;
  c1.bidder_gov = 0;
  c1.star_from = 0;
  c1.planet_from = 0;
  c1.star_to = 0;
  c1.planet_to = 0;
  putcommod(c1, 1);

  Commod c2{};
  c2.owner = 1;
  c2.governor = 0;
  c2.type = CommodType::FUEL;
  c2.amount = 200;
  c2.deliver = true;
  c2.bid = 0;
  c2.bidder = 0;
  c2.bidder_gov = 0;
  c2.star_from = 0;
  c2.planet_from = 0;
  c2.star_to = 0;
  c2.planet_to = 0;
  putcommod(c2, 2);

  Commod c4{};
  c4.owner = 1;
  c4.governor = 0;
  c4.type = CommodType::CRYSTAL;
  c4.amount = 300;
  c4.deliver = true;
  c4.bid = 0;
  c4.bidder = 0;
  c4.bidder_gov = 0;
  c4.star_from = 0;
  c4.planet_from = 0;
  c4.star_to = 0;
  c4.planet_to = 0;
  putcommod(c4, 4);

  int cid2 = getdeadcommod();
  assert(cid2 == 3);
  std::println("✓ Test 7: Commod IDs 1,2,4 -> next is 3 (gap)");

  makecommoddead(1);
  int cid3 = getdeadcommod();
  assert(cid3 == 1);
  std::println("✓ Test 8: Delete commod 1 -> reuses gap at 1");

  std::println("\n✅ All free ID management tests passed!");
  return 0;
}
