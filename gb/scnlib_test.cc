// SPDX-License-Identifier: Apache-2.0

import scnlib;
import std;

#include <cassert>

int main() {
  // Basic scan with integer
  {
    auto result = scn::scan<int>("42", "{}");
    assert(result.has_value());
    auto [value] = result->values();
    assert(value == 42);
    std::println(std::cout, "Test 1 passed: Basic integer scan");
  }

  // Scan with string
  {
    auto result = scn::scan<std::string>("hello", "{}");
    assert(result.has_value());
    auto [value] = result->values();
    assert(value == "hello");
    std::println(std::cout, "Test 2 passed: String scan");
  }

  // Multiple values
  {
    auto result = scn::scan<int, double>("42 3.14", "{} {}");
    assert(result.has_value());
    auto [int_val, double_val] = result->values();
    assert(int_val == 42);
    assert(double_val > 3.13 && double_val < 3.15);
    std::println(std::cout, "Test 3 passed: Multiple value scan");
  }

  // Error handling
  {
    auto result = scn::scan<int>("not_a_number", "{}");
    assert(!result.has_value());
    std::println(std::cout, "Test 4 passed: Error handling for invalid input");
  }

  std::println(std::cout, "\nAll scnlib module tests passed!");
  return 0;
}
