// SPDX-License-Identifier: Apache-2.0

/// \file freeid_test.cc
/// \brief Unit tests for monotonic high-water-mark ID allocation in
/// repositories.

import dallib;
import gb.entities;
import gb.repositories;
import test;
import std;

int main() {
  TestContext ctx;
  ctx.with_standard_universe();
  JsonStore store(ctx.db);
  ShipRepository ship_repo(store);

  std::println(std::cout, "Testing monotonic ship ID management...");

  // Empty table should return 1
  int id1 = ship_repo.next_available_id();
  test::expect_eq(id1, 1);
  std::println(std::cout, "✓ Empty table returns ID 1");

  // Create ships at 1, 2, 3, verify next is 4
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 1;
  ship1.name() = "Ship1";
  ship_repo.save(ship1);

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 1;
  ship2.name() = "Ship2";
  ship_repo.save(ship2);

  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 1;
  ship3.governor() = 1;
  ship3.name() = "Ship3";
  ship_repo.save(ship3);

  int id2 = ship_repo.next_available_id();
  test::expect_eq(id2, 4);
  std::println(std::cout, "✓ Sequential IDs 1,2,3 -> next is 4");

  // Delete middle ship 2, verify next is still 4 (monotonic, no gap reuse)
  ship_repo.delete_ship(2);
  int id3 = ship_repo.next_available_id();
  test::expect_eq(id3, 4);
  std::println(std::cout, "✓ Delete ship 2 -> next remains 4 (no gap reuse)");

  // Delete highest ship 3, verify sqlite_sequence preserves high-water mark 3
  // so next is still 4
  ship_repo.delete_ship(3);
  int id4 = ship_repo.next_available_id();
  test::expect_eq(id4, 4);
  std::println(
      std::cout,
      "✓ Delete highest ship 3 -> next remains 4 via AUTOINCREMENT sequence");

  // Delete remaining ship 1 (table now empty), verify next is still 4
  ship_repo.delete_ship(1);
  int id5 = ship_repo.next_available_id();
  test::expect_eq(id5, 4);
  std::println(std::cout,
               "✓ Delete all ships -> next remains 4 (never reuses dead IDs)");

  // Test commodities work the same way
  std::println(std::cout, "\nTesting monotonic commod ID management...");

  CommodRepository commod_repo(store);

  int cid1 = commod_repo.next_available_id();
  test::expect_eq(cid1, 1);
  std::println(std::cout, "✓ Empty commod table returns ID 1");

  // Create some commods
  Commod c1{{
      .id = 1,
      .owner = 1,
      .governor = 1,
      .type = CommodType::RESOURCE,
      .amount = 100,
      .deliver = true,
      .star_from = 1,
      .planet_from = 1,
  }};
  commod_repo.save(c1);

  Commod c2{{
      .id = 2,
      .owner = 1,
      .governor = 1,
      .type = CommodType::FUEL,
      .amount = 200,
      .deliver = true,
      .star_from = 1,
      .planet_from = 1,
  }};
  commod_repo.save(c2);

  Commod c4{{
      .id = 4,
      .owner = 1,
      .governor = 1,
      .type = CommodType::CRYSTAL,
      .amount = 300,
      .deliver = true,
      .star_from = 1,
      .planet_from = 1,
  }};
  commod_repo.save(c4);

  int cid2 = commod_repo.next_available_id();
  test::expect_eq(cid2, 5);
  std::println(std::cout, "✓ Commod IDs 1,2,4 -> next is 5 (after max)");

  commod_repo.delete_commod(4);
  int cid3 = commod_repo.next_available_id();
  test::expect_eq(cid3, 5);
  std::println(std::cout,
               "✓ Delete highest commod 4 -> next remains 5 (no reuse)");

  std::println(std::cout, "\n✅ All monotonic ID management tests passed!");
  return 0;
}
