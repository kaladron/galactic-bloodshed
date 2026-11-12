// SPDX-License-Identifier: Apache-2.0

/// \file rand_test.cc
/// \brief Unit tests for random number generation functions in gblib:rand.

import gb.entities;
import test;
import std;

int main() {
  std::println(std::cout, "=== Testing Random Number Generator Functions ===");

  // 0. Per-thread random_device seeding (distinct from hardcoded 1337 and
  // across threads) before seed_rand() is called
  {
    std::uint64_t t1_draw = 0;
    std::uint64_t t2_draw = 0;
    {
      std::jthread t1([&] { t1_draw = game_rng()(); });
      std::jthread t2([&] { t2_draw = game_rng()(); });
    }
    std::mt19937_64 fixed_1337(1337);
    const auto fixed_draw = fixed_1337();
    test::expect_ne(t1_draw, fixed_draw);
    test::expect_ne(t2_draw, fixed_draw);
    test::expect_ne(t1_draw, t2_draw);
  }

  seed_rand(42);
  const auto deterministic_first = game_rng()();
  seed_rand(42);
  test::expect_eq(game_rng()(), deterministic_first);
  seed_rand(42);

  // 1. uniform_rand() default [0.0, 1.0)
  for (int i = 0; i < 100; ++i) {
    double r = uniform_rand();
    test::expect_true(r >= 0.0, "uniform_rand() must be >= 0.0");
    test::expect_true(r < 1.0, "uniform_rand() must be < 1.0");
  }

  // 2. uniform_rand(low, high) boundary conditions for floating-point
  test::expect_eq(uniform_rand(5.0, 5.0), 5.0);
  test::expect_eq(uniform_rand(10.0, 2.0), 10.0);

  // 3. uniform_rand(low, high) positive range [10.0, 20.0)
  for (int i = 0; i < 100; ++i) {
    double r = uniform_rand(10.0, 20.0);
    test::expect_true(r >= 10.0, "uniform_rand(10, 20) must be >= 10.0");
    test::expect_true(r < 20.0, "uniform_rand(10, 20) must be < 20.0");
  }

  // 4. uniform_rand(low, high) negative-to-positive range (e.g. launch offset
  // [-10.0, 10.0])
  bool saw_negative = false;
  bool saw_positive = false;
  for (int i = 0; i < 100; ++i) {
    double r = uniform_rand(-10.0, 10.0);
    test::expect_true(r >= -10.0, "uniform_rand(-10, 10) must be >= -10.0");
    test::expect_true(r < 10.0, "uniform_rand(-10, 10) must be < 10.0");
    if (r < 0.0) saw_negative = true;
    if (r > 0.0) saw_positive = true;
  }
  test::expect_true(saw_negative, "Expected negative values in [-10, 10)");
  test::expect_true(saw_positive, "Expected positive values in [-10, 10)");

  // 5. uniform_rand integer bounds
  test::expect_eq(uniform_rand(7, 7), 7);
  test::expect_eq(uniform_rand(10, 3), 10);
  for (int i = 0; i < 100; ++i) {
    int r = uniform_rand(1, 6);
    test::expect_true(r >= 1 && r <= 6, "uniform_rand(1, 6) must be in [1, 6]");
  }

  // 6. bool_rand edge probabilities
  test::expect_false(bool_rand(0.0));
  test::expect_true(bool_rand(1.0));

  // 7. round_rand exact, fractional, non-finite, saturation, integer division,
  // and integer*float scaling
  test::expect_eq(round_rand(0.0), 0);
  test::expect_eq(round_rand(-0.0), 0);
  test::expect_eq(round_rand(5.0), 5);
  test::expect_eq(round_rand(-5.0), -5);
  test::expect_eq(round_rand(std::numeric_limits<double>::quiet_NaN()), 0);
  test::expect_eq(round_rand(std::numeric_limits<double>::infinity()), 0);
  test::expect_eq(round_rand(-std::numeric_limits<double>::infinity()), 0);
  test::expect_eq(round_rand<std::int32_t>(1e20),
                  std::numeric_limits<std::int32_t>::max());
  test::expect_eq(round_rand<std::int32_t>(-1e20),
                  std::numeric_limits<std::int32_t>::min());
  test::expect_eq(round_rand<std::uint32_t>(-0.75), 0U);
  test::expect_eq(round_rand<std::int64_t>(1e30),
                  std::numeric_limits<std::int64_t>::max());
  test::expect_eq(round_rand<std::int64_t>(-1e30),
                  std::numeric_limits<std::int64_t>::min());
  for (int i = 0; i < 50; ++i) {
    int r = round_rand(2.4);
    test::expect_true(r == 2 || r == 3, "round_rand(2.4) must be 2 or 3");

    resource_t res = 10;
    auto div_r = round_rand(res, 3);
    static_assert(std::same_as<decltype(div_r), resource_t>);
    test::expect_true(div_r == 3 || div_r == 4,
                      "round_rand(10, 3) must be 3 or 4");

    population_t pop = 100;
    auto scale_r = round_rand(pop, 0.25);
    static_assert(std::same_as<decltype(scale_r), population_t>);
    test::expect_eq(scale_r, 25);
  }

  // 8. uniform_rand deduced and explicit types with consteval range checks
  test::expect_eq(uniform_rand(7, 7), 7);
  test::expect_eq(uniform_rand(10, 3), 10);
  for (int i = 0; i < 100; ++i) {
    auto r_int = uniform_rand(1, 6);
    static_assert(std::same_as<decltype(r_int), int>);
    test::expect_true(r_int >= 1 && r_int <= 6,
                      "uniform_rand(1, 6) must be in [1, 6]");

    population_t max_pop = 500;
    auto r_pop = uniform_rand(0, max_pop);
    static_assert(std::same_as<decltype(r_pop), population_t>);
    test::expect_true(r_pop >= 0 && r_pop <= 500,
                      "uniform_rand(0, max_pop) must be in [0, 500]");

    constexpr double REPAIR_RATE_BOUND = 25.0;
    auto r_rad = uniform_rand<radiation_t>(0, REPAIR_RATE_BOUND);
    static_assert(std::same_as<decltype(r_rad), radiation_t>);
    test::expect_true(r_rad >= 0 && r_rad <= 25,
                      "uniform_rand<radiation_t>(0, 25.0) must be in [0, 25]");

    auto r_u8 = uniform_rand<std::uint8_t>(0, 10);
    static_assert(std::same_as<decltype(r_u8), std::uint8_t>);
    test::expect_true(r_u8 <= 10, "uniform_rand<uint8_t>(0, 10) must be <= 10");

    auto r_dbl = uniform_rand(-2.5, 2.5);
    static_assert(std::same_as<decltype(r_dbl), double>);
    test::expect_true(r_dbl >= -2.5 && r_dbl < 2.5,
                      "uniform_rand(-2.5, 2.5) must be in [-2.5, 2.5)");
  }

  std::println(std::cout, "✓ All Random Number Generator tests passed!");
  return 0;
}
