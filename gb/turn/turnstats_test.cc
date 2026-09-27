// SPDX-License-Identifier: Apache-2.0

/// \file turnstats_test.cc
/// \brief Unit tests for TurnStats planetary simulation state encapsulation,
/// domain methods, and bounds checking.

import gb.entities;
import gb.turn;
import test;
import std;

namespace {

void test_turnstats_defaults() {
  TurnStats stats{};

  // Check default values at minimum valid 1-based coordinates
  test::expect_eq(stats.temp_add(starnum_t{1}, planetnum_t{1}), 0);
  test::expect_false(stats.is_intimidated(starnum_t{1}, planetnum_t{1}));
  test::expect_false(stats.is_inhabited(starnum_t{1}, planetnum_t{1}));
  test::expect_false(stats.has_alien_colony(starnum_t{1}, planetnum_t{1}));

  // Arbitrary high valid indices work without fixed compile-time bounds
  const starnum_t high_star{500};
  const planetnum_t high_planet{50};
  test::expect_eq(stats.temp_add(high_star, high_planet), 0);
  test::expect_false(stats.is_intimidated(high_star, high_planet));
  test::expect_false(stats.is_inhabited(high_star, high_planet));
  test::expect_false(stats.has_alien_colony(high_star, high_planet));
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
  test::expect_eq(stats.temp_add(starnum_t{1}, planetnum_t{1}), 0);
  test::expect_eq(stats.temp_add(snum, planetnum_t{1}), 0);
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
  const starnum_t zero_star{0};
  const planetnum_t valid_planet{1};
  const starnum_t valid_star{1};
  const planetnum_t zero_planet{0};

  // Star out of bounds (0 is invalid 1-based star ID)
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.temp_add(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_temp_add(zero_star, valid_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_temp(zero_star, valid_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_intimidated(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_intimidated(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_inhabited(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.mark_inhabited(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.has_alien_colony(zero_star, valid_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_alien_colony(zero_star, valid_planet); });

  // Planet out of bounds (0 is invalid 1-based planet ID)
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.temp_add(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_temp_add(valid_star, zero_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_temp(valid_star, zero_planet, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_intimidated(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_intimidated(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.is_inhabited(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.mark_inhabited(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.has_alien_colony(valid_star, zero_planet); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.set_alien_colony(valid_star, zero_planet); });
}

void test_turnstats_record_production() {
  TurnStats stats{};
  const Stockpile batch1{
      .resources = 25, .destruct = 10, .fuel = 50, .crystals = 1};
  const Stockpile batch2{
      .resources = 15, .destruct = 5, .fuel = 30, .crystals = 2};

  stats.record_production(1, batch1);
  stats.record_production(1, batch2);

  const auto& p1_prod = stats.production(1);
  test::expect_eq(p1_prod.resources, 40);
  test::expect_eq(p1_prod.destruct, 15);
  test::expect_eq(p1_prod.fuel, 80);
  test::expect_eq(p1_prod.crystals, 3);
  test::expect_eq(stats.production(2).resources, 0);
}

void test_turnstats_player_and_star_accumulators() {
  TurnStats stats{};

  // Default read-only access returns zero without inserting
  test::expect_eq(stats.star_player_stats(1, 1).popn, 0);
  test::expect_eq(stats.star_player_stats(1, 1).num_ships, 0U);
  test::expect_eq(stats.power_stats(1).popn, 0);
  test::expect_eq(stats.mob_points(1), 0U);
  test::expect_eq(stats.compat(1), 0.0);

  // Mutate and verify arbitrary high IDs
  stats.add_star_popn(500, 100, 8000);
  stats.add_star_ships(500, 100, 12U);
  stats.mutable_power_stats(100).popn = 10000;
  stats.add_mob_points(100, 45U);
  stats.set_compat(100, 85.5);

  test::expect_eq(stats.star_player_stats(500, 100).popn, 8000);
  test::expect_eq(stats.star_player_stats(500, 100).num_ships, 12U);
  test::expect_eq(stats.power_stats(100).popn, 10000);
  test::expect_eq(stats.mob_points(100), 45U);
  test::expect_eq(stats.compat(100), 85.5);

  // reset_planet_accumulators clears planetary production, mob_points, and
  // compat while preserving star_player_stats and power_stats
  stats.record_production(100, Stockpile{.resources = 50});
  stats.reset_planet_accumulators();
  test::expect_eq(stats.production(100).resources, 0);
  test::expect_eq(stats.mob_points(100), 0U);
  test::expect_eq(stats.compat(100), 0.0);
  test::expect_eq(stats.star_player_stats(500, 100).popn, 8000);
  test::expect_eq(stats.power_stats(100).popn, 10000);

  // Bounds checking on 0 IDs
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.star_player_stats(0, 1); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.star_player_stats(1, 0); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_star_popn(0, 1, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_star_popn(1, 0, 10); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_star_ships(0, 1, 1); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_star_ships(1, 0, 1); });
  test::expect_throws<std::out_of_range>([&]() { (void)stats.power_stats(0); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.mutable_power_stats(0); });
  test::expect_throws<std::out_of_range>([&]() { (void)stats.production(0); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.record_production(0, Stockpile{}); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.spend_produced_resources(0, 1); });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.extract_slave_tribute(0); });
  test::expect_throws<std::out_of_range>([&]() { (void)stats.mob_points(0); });
  test::expect_throws<std::out_of_range>(
      [&]() { stats.add_mob_points(0, 10); });
  test::expect_throws<std::out_of_range>([&]() { (void)stats.compat(0); });
  test::expect_throws<std::out_of_range>([&]() { stats.set_compat(0, 1.0); });
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

  std::println(std::cout, "  Testing TurnStats record_production... ");
  test_turnstats_record_production();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing TurnStats player and star accumulators... ");
  test_turnstats_player_and_star_accumulators();
  std::println(std::cout, "PASS");

  std::println(std::cout, "\nAll TurnStats unit tests passed!");
  return 0;
}
