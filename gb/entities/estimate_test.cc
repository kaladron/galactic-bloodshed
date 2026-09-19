// SPDX-License-Identifier: Apache-2.0

/// \file estimate_test.cc
/// \brief Unit tests for estimate() number formatting and translation
/// precision.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race with translation capability
  Race race{};
  race.Playernum = 1;
  race.translate[player_t{1}] = 100;  // Full translation for player 1
  race.translate[player_t{2}] = 50;   // Partial translation for player 2
  race.translate[player_t{3}] = 5;    // Very low translation for player 3

  RaceRepository races(store);
  races.save(race);

  const auto* saved = em.peek_race(1);
  test::expect_ne(saved, nullptr);

  // Test with integer values
  std::string result_int = saved->estimate(42, 1);
  test::expect_eq(result_int, "42");

  std::string result_int_k = saved->estimate(5000, 1);
  test::expect_true(result_int_k == "5.0K" || result_int_k == "5K");

  std::string result_int_m = saved->estimate(1500000, 1);
  test::expect_eq(result_int_m, "1.5M");

  // Test with negative integer
  std::string result_neg = saved->estimate(-42, 1);
  test::expect_eq(result_neg, "42");  // std::abs() is applied in the function

  // Test with double values
  std::string result_double = saved->estimate(42.7, 1);
  test::expect_eq(result_double, "42");

  std::string result_double_k = saved->estimate(5000.5, 1);
  test::expect_true(result_double_k == "5.0K" || result_double_k == "5K");

  std::string result_double_m = saved->estimate(1500000.0, 1);
  test::expect_eq(result_double_m, "1.5M");

  // Test edge case: exactly 1 million
  std::string result_1m = saved->estimate(1000000, 1);
  test::expect_eq(result_1m, "1.0M");

  // Test with low translation
  std::string result_low_trans = saved->estimate(42, 3);
  test::expect_eq(result_low_trans, "?");

  // Test with float type
  float float_val = 42.5f;
  std::string result_float = saved->estimate(float_val, 1);
  test::expect_eq(result_float, "42");

  // Test overload taking const Race&
  std::string result_race_ref = saved->estimate(42, *saved);
  test::expect_eq(result_race_ref, "42");

  std::println(std::cout, "All estimate tests passed!");
  return 0;
}
