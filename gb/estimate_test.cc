// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race with translation capability
  Race race{};
  race.Playernum = 1;
  race.translate[0] = 100;  // Full translation for player 1
  race.translate[1] = 50;   // Partial translation for player 2
  race.translate[2] = 5;    // Very low translation for player 3

  RaceRepository races(store);
  races.save(race);

  const auto* saved = em.peek_race(1);
  assert(saved);

  // Test with integer values
  std::string result_int = estimate(42, *saved, 1);
  assert(result_int == "42");

  std::string result_int_k = estimate(5000, *saved, 1);
  assert(result_int_k == "5.0K" || result_int_k == "5K");

  std::string result_int_m = estimate(1500000, *saved, 1);
  assert(result_int_m == "1.5M");

  // Test with negative integer
  std::string result_neg = estimate(-42, *saved, 1);
  assert(result_neg == "42");  // abs() is applied in the function

  // Test with double values
  std::string result_double = estimate(42.7, *saved, 1);
  assert(result_double == "42");

  std::string result_double_k = estimate(5000.5, *saved, 1);
  assert(result_double_k == "5.0K" || result_double_k == "5K");

  std::string result_double_m = estimate(1500000.0, *saved, 1);
  assert(result_double_m == "1.5M");

  // Test edge case: exactly 1 million
  std::string result_1m = estimate(1000000, *saved, 1);
  assert(result_1m == "1.0M");

  // Test with low translation
  std::string result_low_trans = estimate(42, *saved, 3);
  assert(result_low_trans == "?");

  // Test with float type
  float float_val = 42.5f;
  std::string result_float = estimate(float_val, *saved, 1);
  assert(result_float == "42");

  std::println("All estimate tests passed!");
  return 0;
}
