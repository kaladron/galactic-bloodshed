// SPDX-License-Identifier: Apache-2.0

/// \file invariant_log_test.cc
/// \brief Unit test ensuring log_invariant_violation compiles and executes
/// across diverse types.

import gb.entities;
import gb.services;
import test;
import std;

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

  std::println(std::cout, "✓ Invariant logging test passed!");
  return 0;
}
