// SPDX-License-Identifier: Apache-2.0

/// \file star_repository_test.cc
/// \brief Unit tests for StarRepository CRUD operations and SQLite JSON
/// persistence.

import dallib;
import gb.entities;
import gb.repositories;
import test;
import std;

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

  // Save star
  std::println(std::cout, "Save star...");
  bool saved = repo.save(test_star);
  test::expect_true(saved, "Failed to save star");
  std::println(std::cout, "  ✓ Star saved successfully");

  // Retrieve by star number
  std::println(std::cout, "Retrieve star by number...");
  auto retrieved = repo.find_by_number(1);
  test::expect_true(retrieved.has_value(), "Failed to retrieve star");
  std::println(std::cout, "  ✓ Star retrieved successfully");

  // Verify data integrity using Star accessor methods
  std::println(std::cout, "Verify data integrity...");
  test::expect_eq(retrieved->get_name(), "Sol");
  test::expect_eq(retrieved->xpos(), 100.5);
  test::expect_eq(retrieved->ypos(), 200.75);
  test::expect_eq(retrieved->numplanets(), 8);
  test::expect_eq(retrieved->stability(), 10);
  test::expect_eq(retrieved->nova_stage(), 0);
  test::expect_eq(retrieved->temperature(), 15);
  test::expect_eq(retrieved->gravity(), 1.0);
  test::expect_eq(retrieved->explored(), 0b101010);
  test::expect_eq(retrieved->inhabited(), 0b110011);

  // Verify governor array using accessor (player_t is 1-indexed)
  for (int i = 1; i <= MAXPLAYERS; i++) {
    test::expect_eq(retrieved->governor(player_t{i}), i);
  }

  // Verify AP array - need to get underlying struct for this
  auto retrieved_data = retrieved->get_struct();
  for (int i = 0; i < MAXPLAYERS; i++) {
    test::expect_eq(retrieved_data.AP[i], i * 100);
  }

  // Verify planet names
  for (int i = 0; i < 8; i++) {
    test::expect_eq(retrieved->get_planet_name(i), test_star_data.pnames[i]);
  }
  std::println(std::cout, "  ✓ All fields match original");

  // Update star using Star methods
  std::println(std::cout, "Update star...");
  retrieved->ships() = 100;
  retrieved->temperature() = 20;
  retrieved->stability() = 8;
  saved = repo.save(*retrieved);
  test::expect_true(saved, "Failed to update star");
  std::println(std::cout, "  ✓ Star updated successfully");

  // Retrieve updated star
  std::println(std::cout, "Retrieve updated star...");
  auto updated = repo.find_by_number(1);
  test::expect_true(updated.has_value(), "Failed to retrieve updated star");
  auto updated_data = updated->get_struct();
  test::expect_eq(updated_data.ships, 100);
  test::expect_eq(updated->temperature(), 20);
  test::expect_eq(updated->stability(), 8);
  std::println(std::cout, "  ✓ Updated values verified");

  // Save multiple stars
  std::println(std::cout, "Save multiple stars...");
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

  std::println(std::cout, "  ✓ Multiple stars saved");

  // Retrieve second star
  std::println(std::cout, "Retrieve second star...");
  auto star2_retrieved = repo.find_by_number(2);
  test::expect_true(star2_retrieved.has_value());
  test::expect_eq(star2_retrieved->get_name(), "Alpha Centauri");
  test::expect_eq(star2_retrieved->xpos(), 50.0);
  std::println(std::cout, "  ✓ Second star retrieved correctly");

  // Retrieve third star
  std::println(std::cout, "Retrieve third star...");
  auto star3_retrieved = repo.find_by_number(5);
  test::expect_true(star3_retrieved.has_value());
  test::expect_eq(star3_retrieved->get_name(), "Proxima");
  test::expect_eq(star3_retrieved->xpos(), 200.0);
  std::println(std::cout, "  ✓ Third star retrieved correctly");

  // Next available star number (should find gap at 3)
  std::println(std::cout, "Next available star number...");
  int next_id = repo.next_available_id();
  test::expect_eq(next_id, 3, "Should return 3 (first gap)");
  std::println(std::cout, "  ✓ Next star number is: {}", next_id);

  // Remove a star
  std::println(std::cout, "Remove star...");
  repo.remove(2);
  auto deleted = repo.find_by_number(2);
  test::expect_false(deleted.has_value(), "Star should be deleted");
  std::println(std::cout, "  ✓ Star removed successfully");

  // Find non-existent star
  std::println(std::cout, "Find non-existent star...");
  auto not_found = repo.find_by_number(999);
  test::expect_false(not_found.has_value(),
                     "Should not find non-existent star");
  std::println(std::cout,
               "  ✓ Correctly returns nullopt for non-existent star");

  // List all star IDs
  std::println(std::cout, "List all star IDs...");
  auto ids = repo.list_ids();
  test::expect_eq(ids.size(), 2, "Should have 2 stars after deletion");
  std::println(std::cout, "  ✓ Star count correct: {}", ids.size());

  std::println(std::cout, "\nAll StarRepository tests passed!");
  return 0;
}
