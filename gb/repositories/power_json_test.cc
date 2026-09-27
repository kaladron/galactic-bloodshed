// SPDX-License-Identifier: Apache-2.0

/// \file power_json_test.cc
/// \brief Unit tests for Power entity SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_power table
  initialize_schema(db);

  std::flat_map<player_t, power> test_power;
  for (player_t p : all_players()) {
    test_power[p] = power{.id = p};
  }

  // Initialize some test data for a few players
  test_power[1].id = 1;  // CRITICAL: Set power id
  test_power[1].troops = 1000;
  test_power[1].popn = 50000;
  test_power[1].resource = 25000;
  test_power[1].fuel = 10000;
  test_power[1].destruct = 500;
  test_power[1].ships_owned = 20;
  test_power[1].planets_owned = 3;
  test_power[1].money = 100000;

  test_power[2].id = 2;  // CRITICAL: Set power id
  test_power[2].troops = 800;
  test_power[2].popn = 40000;
  test_power[2].resource = 20000;
  test_power[2].fuel = 8000;
  test_power[2].destruct = 400;
  test_power[2].ships_owned = 15;
  test_power[2].planets_owned = 2;
  test_power[2].money = 80000;

  // Test EntityManager - stores and retrieves power data
  // First save using repository
  JsonStore store(db);
  RaceRepository race_repo(store);
  PowerRepository power_repo(store);
  for (player_t p : all_players()) {
    Race r{};
    r.Playernum = p;
    race_repo.save(r);
    power_repo.save(test_power[p]);
  }

  // Now use EntityManager to retrieve
  EntityManager em(db);
  std::flat_map<player_t, power> loaded_power;

  // Retrieve from EntityManager
  for (player_t p : all_players()) {
    const auto* power_ptr = em.peek_power(p);
    test::expect_ne(power_ptr, nullptr);  // Should exist now
    loaded_power[p] = *power_ptr;
  }

  // Verify the data matches
  test::expect_eq(loaded_power[1].troops, test_power[1].troops);
  test::expect_eq(loaded_power[1].popn, test_power[1].popn);
  test::expect_eq(loaded_power[1].resource, test_power[1].resource);
  test::expect_eq(loaded_power[1].fuel, test_power[1].fuel);
  test::expect_eq(loaded_power[1].destruct, test_power[1].destruct);
  test::expect_eq(loaded_power[1].ships_owned, test_power[1].ships_owned);
  test::expect_eq(loaded_power[1].planets_owned, test_power[1].planets_owned);
  test::expect_eq(loaded_power[1].money, test_power[1].money);

  test::expect_eq(loaded_power[2].troops, test_power[2].troops);
  test::expect_eq(loaded_power[2].popn, test_power[2].popn);
  test::expect_eq(loaded_power[2].resource, test_power[2].resource);
  test::expect_eq(loaded_power[2].fuel, test_power[2].fuel);
  test::expect_eq(loaded_power[2].destruct, test_power[2].destruct);
  test::expect_eq(loaded_power[2].ships_owned, test_power[2].ships_owned);
  test::expect_eq(loaded_power[2].planets_owned, test_power[2].planets_owned);
  test::expect_eq(loaded_power[2].money, test_power[2].money);

  std::println(std::cout, "All power JSON serialization tests passed!");
  return 0;
}
