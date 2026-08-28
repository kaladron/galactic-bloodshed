// SPDX-License-Identifier: Apache-2.0

/// \file scnlib_test.cc
/// \brief Unit tests for scnlib module wrapper (integer, string, multiple
/// values, and invalid scan handling).

import scnlib;
import test;
import std;

int main() {
  // Basic scan with integer
  {
    auto result = scn::scan<int>("42", "{}");
    test::expect_true(result.has_value());
    auto [value] = result->values();
    test::expect_eq(value, 42);
    std::println(std::cout, "Test 1 passed: Basic integer scan");
  }

  // Scan with string
  {
    auto result = scn::scan<std::string>("hello", "{}");
    test::expect_true(result.has_value());
    auto [value] = result->values();
    test::expect_eq(value, "hello");
    std::println(std::cout, "Test 2 passed: String scan");
  }

  // Multiple values
  {
    auto result = scn::scan<int, double>("42 3.14", "{} {}");
    test::expect_true(result.has_value());
    auto [int_val, double_val] = result->values();
    test::expect_eq(int_val, 42);
    test::expect_gt(double_val, 3.13);
    test::expect_lt(double_val, 3.15);
    std::println(std::cout, "Test 3 passed: Multiple value scan");
  }

  // Error handling
  {
    auto result = scn::scan<int>("not_a_number", "{}");
    test::expect_false(result.has_value());
    std::println(std::cout, "Test 4 passed: Error handling for invalid input");
  }

  std::println(std::cout, "\nAll scnlib module tests passed!");
  return 0;
}
