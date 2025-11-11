// SPDX-License-Identifier: Apache-2.0

/// \file motto_test.cc
/// \brief Test motto command database persistence

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

void test_motto_database_persistence() {
  std::println("Test: motto command database persistence");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a block with initial empty motto
  block b{};
  b.Playernum = 1;
  std::strncpy(b.name, "Test Alliance", sizeof(b.name) - 1);
  b.motto[0] = '\0';  // Initially empty

  JsonStore store(db);
  BlockRepository blocks(store);
  blocks.save(b);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;  // Must be governor 0 to set motto

  // TEST 1: Set a motto
  std::println("  Testing: Set motto");
  {
    command_t cmd = {"motto", "For", "the", "Empire!"};
    GB::commands::motto(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("Done") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output

    // Verify database: motto should be set
    auto saved = blocks.find_by_id(1);
    assert(saved.has_value());
    std::string saved_motto = saved->motto;
    assert(saved_motto.find("For the Empire!") != std::string::npos);
    std::println("    ✓ Database: motto = '{}'", saved->motto);
  }

  // TEST 2: Change the motto
  std::println("  Testing: Change motto");
  {
    command_t cmd = {"motto", "Victory", "or", "Death"};
    GB::commands::motto(cmd, g);

    // Verify database: motto should be changed
    auto saved = blocks.find_by_id(1);
    assert(saved.has_value());
    std::string saved_motto = saved->motto;
    assert(saved_motto.find("Victory or Death") != std::string::npos);
    std::println("    ✓ Database: motto = '{}'", saved->motto);
  }

  // TEST 3: Set empty motto
  std::println("  Testing: Clear motto with single space");
  {
    command_t cmd = {"motto", " "};
    GB::commands::motto(cmd, g);

    auto saved = blocks.find_by_id(1);
    assert(saved.has_value());
    std::println("    ✓ Database: motto = '{}'", saved->motto);
  }

  // TEST 4: Non-governor should be rejected
  std::println("  Testing: Non-governor authorization check");
  {
    g.governor = 1;  // Change to non-zero governor
    command_t cmd = {"motto", "Should", "Fail"};
    GB::commands::motto(cmd, g);

    std::string out_str = g.out.str();
    assert(out_str.find("not authorized") != std::string::npos);
    std::println("    ✓ Authorization check works");
    g.out.str("");
  }

  std::println("  ✅ All motto database persistence tests passed!");
}

int main() {
  test_motto_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
