// SPDX-License-Identifier: Apache-2.0

/// \file invariant_log_test.cc
/// \brief Unit test ensuring log_invariant_violation compiles and executes
/// across diverse types.

import gb.entities;
import gb.log;
import gb.services;
import test;
import std;

int main() {
  std::ostringstream captured;
  {
    GB::log::ScopedSink sink{captured};

    // Test with integer types
    log_invariant_violation("Sector", "popn", -100, 0);

    // Test with mixed types
    log_invariant_violation("Planet", "temp", 150, 100);

    // Test with double types
    log_invariant_violation("Ship", "fuel", -5.5, 0.0);

    // Test with source location (implicit)
    log_invariant_violation("Race", "tech", -1.0, 0.0);
  }

  const std::string out = captured.str();
  test::expect_true(out.starts_with("<4>"));
  test::expect_contains(
      out, "[WARN] [INVARIANT] Sector::popn: attempted -100, clamped to 0");
  test::expect_contains(
      out, "[WARN] [INVARIANT] Planet::temp: attempted 150, clamped to 100");
  test::expect_contains(
      out, "[WARN] [INVARIANT] Ship::fuel: attempted -5.5, clamped to 0");
  test::expect_contains(
      out, "[WARN] [INVARIANT] Race::tech: attempted -1, clamped to 0");

  std::println(std::cout, "✓ Invariant logging test passed!");
  return 0;
}
