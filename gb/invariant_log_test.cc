// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

int main() {
  // Test that log_invariant_violation compiles and can be called
  // This is a compile-time test - we're just ensuring the API works

  // Test with integer types
  log_invariant_violation("Sector", "popn", -100, 0);

  // Test with mixed types
  log_invariant_violation("Planet", "temp", 150, 100);

  // Test with double types
  log_invariant_violation("Ship", "fuel", -5.5, 0.0);

  // Test with source location (implicit)
  log_invariant_violation("Race", "tech", -1.0, 0.0);

  std::println("Invariant logging test passed!");
  std::println("Note: If kDebugInvariants is true, you should see [INVARIANT] "
               "messages above.");

  return 0;
}
