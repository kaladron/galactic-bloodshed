// SPDX-License-Identifier: Apache-2.0

import scnlib;
import std;

#include <cassert>

int main() {
  // Test 1: Basic scan with integer
  {
    auto result = scn::scan<int>("42", "{}");
    assert(result.has_value());
    auto [value] = result->values();
    assert(value == 42);
    std::println("Test 1 passed: Basic integer scan");
  }

  // Test 2: Scan with string
  {
    auto result = scn::scan<std::string>("hello", "{}");
    assert(result.has_value());
    auto [value] = result->values();
    assert(value == "hello");
    std::println("Test 2 passed: String scan");
  }

  // Test 3: Multiple values
  {
    auto result = scn::scan<int, double>("42 3.14", "{} {}");
    assert(result.has_value());
    auto [int_val, double_val] = result->values();
    assert(int_val == 42);
    assert(double_val > 3.13 && double_val < 3.15);
    std::println("Test 3 passed: Multiple value scan");
  }

  // Test 4: Error handling
  {
    auto result = scn::scan<int>("not_a_number", "{}");
    assert(!result.has_value());
    std::println("Test 4 passed: Error handling for invalid input");
  }

  std::println("\nAll scnlib module tests passed!");
  return 0;
}
