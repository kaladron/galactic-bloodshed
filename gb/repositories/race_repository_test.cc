// SPDX-License-Identifier: Apache-2.0

import gblib;
import dallib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and RaceRepository
  JsonStore store(db);
  RaceRepository repo(store);

  // Create a test race
  Race test_race{};
  test_race.Playernum = 1;
  test_race.name = "Test Race";
  test_race.password = "secret123";
  test_race.info = "A test civilization";
  test_race.motto = "Testing is believing";
  test_race.absorb = true;
  test_race.collective_iq = false;
  test_race.pods = true;
  test_race.fighters = 75;
  test_race.IQ = 150;
  test_race.IQ_limit = 200;
  test_race.number_sexes = 2;
  test_race.fertilize = 10;
  test_race.adventurism = 0.5;
  test_race.birthrate = 0.15;
  test_race.mass = 1.0;
  test_race.metabolism = 1.0;
  test_race.dissolved = false;
  test_race.God = false;
  test_race.Guest = false;
  test_race.Metamorph = false;
  test_race.monitor = false;
  test_race.Gov_ship = 100;
  test_race.morale = 1000;
  test_race.controlled_planets = 5;
  test_race.victory_turns = 0;
  test_race.turn = 42;
  test_race.tech = 25.5;
  test_race.victory_score = 5000;
  test_race.votes = true;
  test_race.planet_points = 100;
  test_race.governors = 3;

  // Initialize some arrays
  for (int i = 0; i <= OTHER; ++i) {
    test_race.conditions[i] = 50 + i;
  }
  for (int i = 0; i <= SectorType::SEC_WASTED; ++i) {
    test_race.likes[i] = 0.5 + (i * 0.1);
  }
  test_race.likesbest = SectorType::SEC_SEA;

  // Initialize governor data
  test_race.governor[0].name = "Governor Zero";
  test_race.governor[0].password = "gov0pass";
  test_race.governor[0].active = true;
  test_race.governor[0].money = 10000;
  test_race.governor[0].income = 5000;

  // Test 1: Save race
  std::println("Test 1: Save race...");
  bool saved = repo.save_race(test_race);
  assert(saved && "Failed to save race");
  std::println("  ✓ Race saved successfully");

  // Test 2: Retrieve by player number
  std::println("Test 2: Retrieve race by player number...");
  auto retrieved = repo.find_by_player(1);
  assert(retrieved.has_value() && "Failed to retrieve race");
  std::println("  ✓ Race retrieved successfully");

  // Test 3: Verify data integrity
  std::println("Test 3: Verify data integrity...");
  assert(retrieved->Playernum == test_race.Playernum);
  assert(retrieved->name == test_race.name);
  assert(retrieved->password == test_race.password);
  assert(retrieved->info == test_race.info);
  assert(retrieved->motto == test_race.motto);
  assert(retrieved->absorb == test_race.absorb);
  assert(retrieved->collective_iq == test_race.collective_iq);
  assert(retrieved->pods == test_race.pods);
  assert(retrieved->fighters == test_race.fighters);
  assert(retrieved->IQ == test_race.IQ);
  assert(retrieved->tech == test_race.tech);
  assert(retrieved->governors == test_race.governors);
  assert(retrieved->governor[0].name == test_race.governor[0].name);
  assert(retrieved->governor[0].money == test_race.governor[0].money);
  std::println("  ✓ All fields match original");

  // Test 4: Update race
  std::println("Test 4: Update race...");
  retrieved->tech = 50.0;
  retrieved->morale = 2000;
  saved = repo.save_race(*retrieved);
  assert(saved && "Failed to update race");
  std::println("  ✓ Race updated successfully");

  // Test 5: Retrieve updated race
  std::println("Test 5: Retrieve updated race...");
  auto updated = repo.find_by_player(1);
  assert(updated.has_value() && "Failed to retrieve updated race");
  assert(updated->tech == 50.0);
  assert(updated->morale == 2000);
  std::println("  ✓ Updated values verified");

  // Test 6: Find non-existent race
  std::println("Test 6: Find non-existent race...");
  auto not_found = repo.find_by_player(99);
  assert(!not_found.has_value() && "Should not find non-existent race");
  std::println("  ✓ Correctly returns nullopt for non-existent race");

  std::println("\nAll RaceRepository tests passed!");
  return 0;
}
