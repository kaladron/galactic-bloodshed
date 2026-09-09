// SPDX-License-Identifier: Apache-2.0

/// \file turnstats_test.cc
/// \brief Unit tests for TurnStats planetary simulation state encapsulation,
/// domain methods, and bounds checking.

import gb.entities;
import test;
import std;

namespace {

void test_turnstats_defaults() {
  TurnStats stats{};

  // Check default values at origin and arbitrary coordinates
  test::expect_eq(stats.temp_add(starnum_t{0}, planetnum_t{0}), 0);
  test::expect_false(stats.is_intimidated(starnum_t{0}, planetnum_t{0}));
  test::expect_false(stats.is_inhabited(starnum_t{0}, planetnum_t{0}));
  test::expect_false(stats.has_alien_colony(starnum_t{0}, planetnum_t{0}));

  // Maximum valid indices
  const starnum_t max_star{NUMSTARS - 1};
  const planetnum_t max_planet{MAXPLANETS - 1};
  test::expect_eq(stats.temp_add(max_star, max_planet), 0);
  test::expect_false(stats.is_intimidated(max_star, max_planet));
  test::expect_false(stats.is_inhabited(max_star, max_planet));
  test::expect_false(stats.has_alien_colony(max_star, max_planet));
}

void test_turnstats_temperature_operations() {
  TurnStats stats{};
  const starnum_t snum{2};
  const planetnum_t pnum{3};

  // Direct set
  stats.set_temp_add(snum, pnum, 25);
  test::expect_eq(stats.temp_add(snum, pnum), 25);

  // Delta additions
  stats.add_temp(snum, pnum, 15);
  test::expect_eq(stats.temp_add(snum, pnum), 40);

  stats.add_temp(snum, pnum, -60);
  test::expect_eq(stats.temp_add(snum, pnum), -20);

  // Other planets unaffected
  test::expect_eq(stats.temp_add(starnum_t{0}, planetnum_t{0}), 0);
  test::expect_eq(stats.temp_add(snum, planetnum_t{0}), 0);
}

void test_turnstats_status_flags() {
  TurnStats stats{};
  const starnum_t snum{5};
  const planetnum_t pnum{2};

  // Intimidation flag
  stats.set_intimidated(snum, pnum);
  test::expect_true(stats.is_intimidated(snum, pnum));
  stats.set_intimidated(snum, pnum, false);
  test::expect_false(stats.is_intimidated(snum, pnum));

  // Inhabitation flag
  stats.mark_inhabited(snum, pnum);
  test::expect_true(stats.is_inhabited(snum, pnum));
  stats.mark_inhabited(snum, pnum, false);
  test::expect_false(stats.is_inhabited(snum, pnum));

  // Alien colony flag
  stats.set_alien_colony(snum, pnum);
  test::expect_true(stats.has_alien_colony(snum, pnum));
  stats.set_alien_colony(snum, pnum, false);
  test::expect_false(stats.has_alien_colony(snum, pnum));
}

void test_turnstats_bounds_checking() {
  TurnStats stats{};
  const starnum_t out_star{NUMSTARS};
  const planetnum_t valid_planet{0};
  const starnum_t valid_star{0};
  const planetnum_t out_planet{MAXPLANETS};

  // Star out of bounds
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.temp_add(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_temp_add(out_star, valid_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_temp(out_star, valid_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_intimidated(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_intimidated(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_inhabited(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.mark_inhabited(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.has_alien_colony(out_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_alien_colony(out_star, valid_planet); });

  // Planet out of bounds
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.temp_add(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_temp_add(valid_star, out_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_temp(valid_star, out_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_intimidated(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_intimidated(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_inhabited(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.mark_inhabited(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.has_alien_colony(valid_star, out_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_alien_colony(valid_star, out_planet); });
}

}  // namespace

int main() {
  std::println(std::cout, "Running TurnStats unit tests...\n");

  std::println(std::cout, "  Testing TurnStats defaults... ");
  test_turnstats_defaults();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing TurnStats temperature operations... ");
  test_turnstats_temperature_operations();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing TurnStats status flags... ");
  test_turnstats_status_flags();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing TurnStats bounds checking... ");
  test_turnstats_bounds_checking();
  std::println(std::cout, "PASS");

  std::println(std::cout, "\nAll TurnStats unit tests passed!");
  return 0;
}
