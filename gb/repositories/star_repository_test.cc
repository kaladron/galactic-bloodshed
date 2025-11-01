// SPDX-License-Identifier: Apache-2.0

import gblib;
import dallib;
import std.compat;

#include <cassert>
#include <cstring>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and StarRepository
  JsonStore store(db);
  StarRepository repo(store);

  // Create a test star
  star_struct test_star{};
  test_star.ships = 42;
  std::strncpy(test_star.name, "Sol", NAMESIZE - 1);
  test_star.xpos = 100.5;
  test_star.ypos = 200.75;
  test_star.numplanets = 8;
  test_star.stability = 10;
  test_star.nova_stage = 0;
  test_star.temperature = 15;
  test_star.gravity = 1.0;
  test_star.star_id = 1;
  test_star.explored = 0b101010;
  test_star.inhabited = 0b110011;

  // Initialize governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.governor[i] = i + 1;
  }

  // Initialize AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.AP[i] = i * 100;
  }

  // Initialize planet names
  std::strncpy(test_star.pnames[0], "Mercury", NAMESIZE - 1);
  std::strncpy(test_star.pnames[1], "Venus", NAMESIZE - 1);
  std::strncpy(test_star.pnames[2], "Earth", NAMESIZE - 1);
  std::strncpy(test_star.pnames[3], "Mars", NAMESIZE - 1);
  std::strncpy(test_star.pnames[4], "Jupiter", NAMESIZE - 1);
  std::strncpy(test_star.pnames[5], "Saturn", NAMESIZE - 1);
  std::strncpy(test_star.pnames[6], "Uranus", NAMESIZE - 1);
  std::strncpy(test_star.pnames[7], "Neptune", NAMESIZE - 1);

  // Test 1: Save star
  std::println("Test 1: Save star...");
  bool saved = repo.save(test_star);
  assert(saved && "Failed to save star");
  std::println("  ✓ Star saved successfully");

  // Test 2: Retrieve by star number
  std::println("Test 2: Retrieve star by number...");
  auto retrieved = repo.find_by_number(1);
  assert(retrieved.has_value() && "Failed to retrieve star");
  std::println("  ✓ Star retrieved successfully");

  // Test 3: Verify data integrity
  std::println("Test 3: Verify data integrity...");
  assert(retrieved->ships == test_star.ships);
  assert(strcmp(retrieved->name, test_star.name) == 0);
  assert(retrieved->xpos == test_star.xpos);
  assert(retrieved->ypos == test_star.ypos);
  assert(retrieved->numplanets == test_star.numplanets);
  assert(retrieved->stability == test_star.stability);
  assert(retrieved->nova_stage == test_star.nova_stage);
  assert(retrieved->temperature == test_star.temperature);
  assert(retrieved->gravity == test_star.gravity);
  assert(retrieved->star_id == test_star.star_id);
  assert(retrieved->explored == test_star.explored);
  assert(retrieved->inhabited == test_star.inhabited);

  // Verify governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved->governor[i] == test_star.governor[i]);
  }

  // Verify AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved->AP[i] == test_star.AP[i]);
  }

  // Verify planet names
  for (int i = 0; i < test_star.numplanets; i++) {
    assert(strcmp(retrieved->pnames[i], test_star.pnames[i]) == 0);
  }
  std::println("  ✓ All fields match original");

  // Test 4: Update star
  std::println("Test 4: Update star...");
  retrieved->ships = 100;
  retrieved->temperature = 20;
  retrieved->stability = 8;
  saved = repo.save(*retrieved);
  assert(saved && "Failed to update star");
  std::println("  ✓ Star updated successfully");

  // Test 5: Retrieve updated star
  std::println("Test 5: Retrieve updated star...");
  auto updated = repo.find_by_number(1);
  assert(updated.has_value() && "Failed to retrieve updated star");
  assert(updated->ships == 100);
  assert(updated->temperature == 20);
  assert(updated->stability == 8);
  std::println("  ✓ Updated values verified");

  // Test 6: Save multiple stars
  std::println("Test 6: Save multiple stars...");
  star_struct star2 = test_star;
  star2.star_id = 2;
  std::strncpy(star2.name, "Alpha Centauri", NAMESIZE - 1);
  star2.xpos = 50.0;
  star2.ypos = 75.0;
  repo.save(star2);

  star_struct star3 = test_star;
  star3.star_id = 5;  // Gap at 3 and 4
  std::strncpy(star3.name, "Proxima", NAMESIZE - 1);
  star3.xpos = 200.0;
  star3.ypos = 150.0;
  repo.save(star3);

  std::println("  ✓ Multiple stars saved");

  // Test 7: Retrieve second star
  std::println("Test 7: Retrieve second star...");
  auto star2_retrieved = repo.find_by_number(2);
  assert(star2_retrieved.has_value());
  assert(strcmp(star2_retrieved->name, "Alpha Centauri") == 0);
  assert(star2_retrieved->xpos == 50.0);
  std::println("  ✓ Second star retrieved correctly");

  // Test 8: Retrieve third star
  std::println("Test 8: Retrieve third star...");
  auto star3_retrieved = repo.find_by_number(5);
  assert(star3_retrieved.has_value());
  assert(strcmp(star3_retrieved->name, "Proxima") == 0);
  assert(star3_retrieved->xpos == 200.0);
  std::println("  ✓ Third star retrieved correctly");

  // Test 9: Next available star number (should find gap at 3)
  std::println("Test 9: Next available star number...");
  int next_id = repo.next_available_id();
  assert(next_id == 3 && "Should return 3 (first gap)");
  std::println("  ✓ Next star number is: {}", next_id);

  // Test 10: Remove a star
  std::println("Test 10: Remove star...");
  repo.remove(2);
  auto deleted = repo.find_by_number(2);
  assert(!deleted.has_value() && "Star should be deleted");
  std::println("  ✓ Star removed successfully");

  // Test 11: Find non-existent star
  std::println("Test 11: Find non-existent star...");
  auto not_found = repo.find_by_number(999);
  assert(!not_found.has_value() && "Should not find non-existent star");
  std::println("  ✓ Correctly returns nullopt for non-existent star");

  // Test 12: List all star IDs
  std::println("Test 12: List all star IDs...");
  auto ids = repo.list_ids();
  assert(ids.size() == 2 && "Should have 2 stars after deletion");
  std::println("  ✓ Star count correct: {}", ids.size());

  std::println("\nAll StarRepository tests passed!");
  return 0;
}
