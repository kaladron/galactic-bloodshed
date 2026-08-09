// SPDX-License-Identifier: Apache-2.0

import std;
import dallib;
import gblib;

#include <cassert>

#include "gb/racegen.h"

int enroll_valid_race(Database& db);

/// \file enroll_test.cc
/// \brief Test race enrollment validation rules

// Global variable definitions required by GB_racegen.cc
struct x race_info{};
const char* planet_print_name[N_HOME_PLANET_TYPES] = {
    "Earth", "Forest", "Desert", "Water", "Airless", "Iceball", "Jovian"};
const double planet_compat_cov[N_HOME_PLANET_TYPES][N_SECTOR_TYPES] = {
    {1.00, 1.00, 2.00, 99.00, 1.01, 1.50, 3.00, 1.01},
    {1.01, 1.50, 2.00, 99.00, 1.01, 1.00, 3.00, 1.01},
    {3.00, 1.01, 1.01, 99.00, 1.50, 3.00, 1.00, 1.01},
    {1.00, 1.50, 3.00, 99.00, 1.01, 1.01, 3.00, 1.01},
    {1.01, 1.00, 1.00, 99.00, 1.01, 1.01, 1.00, 1.01},
    {3.00, 1.01, 1.00, 99.00, 1.00, 1.50, 2.00, 1.01},
    {99.00, 99.00, 99.00, 1.00, 99.00, 99.00, 99.00, 99.00}};

void test_enroll_first_race_god_requirement() {
  std::println(std::cout, "Test: First race enrolled must be God");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);

  // Setup: Set race_info to non-God race
  race_info = x{};
  race_info.priv_type = P_NORMAL;

  // TEST: Attempt to enroll non-God race as player 1
  int result = enroll_valid_race(db);

  // Verify: Enrollment fails with God privilege error
  assert(result == 1);
  assert(std::string(race_info.rejection)
             .find("The first race enrolled must have God privileges.") !=
         std::string::npos);

  std::println(std::cout, "  ✓ God race requirement check passed");
}

void test_enroll_max_players() {
  std::println(std::cout, "Test: Max player limit enforcement");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);

  // Setup: Save MAXPLAYERS-1 dummy races to fill the database
  JsonStore store(db);
  RaceRepository races(store);

  for (int i = 1; i < MAXPLAYERS; ++i) {
    Race r{};
    r.Playernum = i;
    r.name = std::format("Race{}", i);
    races.save(r);
  }

  // Setup: Prepare new race for enrollment
  race_info = x{};
  race_info.priv_type = P_GOD;

  // TEST: Attempt to enroll when MAXPLAYERS is reached
  int result = enroll_valid_race(db);

  // Verify: Enrollment rejected due to max players limit
  assert(result == 1);
  assert(race_info.status == STATUS_UNENROLLABLE);
  assert(std::string(race_info.rejection).find("No more allowed.") !=
         std::string::npos);

  std::println(std::cout, "  ✓ Max player limit enforcement passed");
}

int main() {
  test_enroll_first_race_god_requirement();
  test_enroll_max_players();

  std::println(std::cout, "\n✅ All enroll tests passed!");
  return 0;
}
