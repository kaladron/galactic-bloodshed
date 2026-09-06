// SPDX-License-Identifier: Apache-2.0

/// \file rand_test.cc
/// \brief Unit tests for random number generation functions in gblib:rand.

import gb.entities;
import test;
import std;

int main() {
  std::println(std::cout, "=== Testing Random Number Generator Functions ===");

  seed_rand(42);

  // 1. double_rand() default [0.0, 1.0)
  for (int i = 0; i < 100; ++i) {
    double r = double_rand();
    test::expect_true(r >= 0.0, "double_rand() must be >= 0.0");
    test::expect_true(r < 1.0, "double_rand() must be < 1.0");
  }

  // 2. double_rand(low, high) boundary conditions
  test::expect_eq(double_rand(5.0, 5.0), 5.0);
  test::expect_eq(double_rand(10.0, 2.0), 10.0);

  // 3. double_rand(low, high) positive range [10.0, 20.0)
  for (int i = 0; i < 100; ++i) {
    double r = double_rand(10.0, 20.0);
    test::expect_true(r >= 10.0, "double_rand(10, 20) must be >= 10.0");
    test::expect_true(r < 20.0, "double_rand(10, 20) must be < 20.0");
  }

  // 4. double_rand(low, high) negative-to-positive range (e.g. launch offset
  // [-10.0, 10.0])
  bool saw_negative = false;
  bool saw_positive = false;
  for (int i = 0; i < 100; ++i) {
    double r = double_rand(-10.0, 10.0);
    test::expect_true(r >= -10.0, "double_rand(-10, 10) must be >= -10.0");
    test::expect_true(r < 10.0, "double_rand(-10, 10) must be < 10.0");
    if (r < 0.0) saw_negative = true;
    if (r > 0.0) saw_positive = true;
  }
  test::expect_true(saw_negative, "Expected negative values in [-10, 10)");
  test::expect_true(saw_positive, "Expected positive values in [-10, 10)");

  // 5. int_rand bounds
  test::expect_eq(int_rand(7, 7), 7);
  test::expect_eq(int_rand(10, 3), 10);
  for (int i = 0; i < 100; ++i) {
    int r = int_rand(1, 6);
    test::expect_true(r >= 1 && r <= 6, "int_rand(1, 6) must be in [1, 6]");
  }

  // 6. bool_rand edge probabilities
  test::expect_false(bool_rand(0.0));
  test::expect_true(bool_rand(1.0));

  // 7. round_rand exact and fractional
  test::expect_eq(round_rand(0.0), 0);
  test::expect_eq(round_rand(5.0), 5);
  for (int i = 0; i < 50; ++i) {
    int r = round_rand(2.4);
    test::expect_true(r == 2 || r == 3, "round_rand(2.4) must be 2 or 3");
  }

  std::println(std::cout, "✓ All Random Number Generator tests passed!");
  return 0;
}
