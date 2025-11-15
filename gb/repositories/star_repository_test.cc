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

  // Create a test star_struct first, then wrap in Star
  star_struct test_star_data{};
  test_star_data.ships = 42;
  test_star_data.name = "Sol";
  test_star_data.xpos = 100.5;
  test_star_data.ypos = 200.75;
  test_star_data.stability = 10;
  test_star_data.nova_stage = 0;
  test_star_data.temperature = 15;
  test_star_data.gravity = 1.0;
  test_star_data.star_id = 1;
  test_star_data.explored = 0b101010;
  test_star_data.inhabited = 0b110011;

  // Initialize governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star_data.governor[i] = i + 1;
  }

  // Initialize AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star_data.AP[i] = i * 100;
  }

  // Initialize planet names using vector
  test_star_data.pnames.push_back("Mercury");
  test_star_data.pnames.push_back("Venus");
  test_star_data.pnames.push_back("Earth");
  test_star_data.pnames.push_back("Mars");
  test_star_data.pnames.push_back("Jupiter");
  test_star_data.pnames.push_back("Saturn");
  test_star_data.pnames.push_back("Uranus");
  test_star_data.pnames.push_back("Neptune");

  // Wrap in Star object
  Star test_star(test_star_data);

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

  // Test 3: Verify data integrity using Star accessor methods
  std::println("Test 3: Verify data integrity...");
  assert(retrieved->get_name() == "Sol");
  assert(retrieved->xpos() == 100.5);
  assert(retrieved->ypos() == 200.75);
  assert(retrieved->numplanets() == 8);
  assert(retrieved->stability() == 10);
  assert(retrieved->nova_stage() == 0);
  assert(retrieved->temperature() == 15);
  assert(retrieved->gravity() == 1.0);
  assert(retrieved->explored() == 0b101010);
  assert(retrieved->inhabited() == 0b110011);

  // Verify governor array using accessor
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved->governor(i) == i + 1);
  }

  // Verify AP array - need to get underlying struct for this
  auto retrieved_data = retrieved->get_struct();
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved_data.AP[i] == i * 100);
  }

  // Verify planet names
  for (int i = 0; i < 8; i++) {
    assert(retrieved->get_planet_name(i) == test_star_data.pnames[i]);
  }
  std::println("  ✓ All fields match original");

  // Test 4: Update star using Star methods
  std::println("Test 4: Update star...");
  retrieved->ships() = 100;
  retrieved->temperature() = 20;
  retrieved->stability() = 8;
  saved = repo.save(*retrieved);
  assert(saved && "Failed to update star");
  std::println("  ✓ Star updated successfully");

  // Test 5: Retrieve updated star
  std::println("Test 5: Retrieve updated star...");
  auto updated = repo.find_by_number(1);
  assert(updated.has_value() && "Failed to retrieve updated star");
  auto updated_data = updated->get_struct();
  assert(updated_data.ships == 100);
  assert(updated->temperature() == 20);
  assert(updated->stability() == 8);
  std::println("  ✓ Updated values verified");

  // Test 6: Save multiple stars
  std::println("Test 6: Save multiple stars...");
  star_struct star2_data = test_star_data;
  star2_data.star_id = 2;
  star2_data.name = "Alpha Centauri";
  star2_data.xpos = 50.0;
  star2_data.ypos = 75.0;
  Star star2(star2_data);
  repo.save(star2);

  star_struct star3_data = test_star_data;
  star3_data.star_id = 5;  // Gap at 3 and 4
  star3_data.name = "Proxima";
  star3_data.xpos = 200.0;
  star3_data.ypos = 150.0;
  Star star3(star3_data);
  repo.save(star3);

  std::println("  ✓ Multiple stars saved");

  // Test 7: Retrieve second star
  std::println("Test 7: Retrieve second star...");
  auto star2_retrieved = repo.find_by_number(2);
  assert(star2_retrieved.has_value());
  assert(star2_retrieved->get_name() == "Alpha Centauri");
  assert(star2_retrieved->xpos() == 50.0);
  std::println("  ✓ Second star retrieved correctly");

  // Test 8: Retrieve third star
  std::println("Test 8: Retrieve third star...");
  auto star3_retrieved = repo.find_by_number(5);
  assert(star3_retrieved.has_value());
  assert(star3_retrieved->get_name() == "Proxima");
  assert(star3_retrieved->xpos() == 200.0);
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
