// SPDX-License-Identifier: Apache-2.0

/// \file estimate_test.cc
/// \brief Unit tests for estimate() number formatting and translation
/// precision.

import dallib;
import gb.entities;
import gb.mechanics;
import gb.services;
import gb.turn;
import test;
import std;

namespace {

void test_ship_visibility_helpers() {
  TestContext ctx;
  const auto races = ctx.with_races();
  const player_t viewer = races.empire;
  const player_t foreign = races.newcomer;

  // Initially neither mortal race has a ship in Vega (Star 2)
  test::expect_false(has_system_sight(ctx.em, 2, viewer, false));
  test::expect_true(has_system_sight(ctx.em, 2, viewer, true));
  test::expect_false(has_planet_sight(ctx.em, 2, 1, viewer, false));
  test::expect_true(has_planet_sight(ctx.em, 2, 1, viewer, true));

  // Add a viewer cruiser with crew (has_sight() == true) in Vega star orbit
  const shipnum_t p1_cruiser = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                   .owned_by(viewer, 1)
                                   .in_star_orbit(2)
                                   .with_crew(50, 10)
                                   .build();
  test::expect_true(has_system_sight(ctx.em, 2, viewer, false));
  test::expect_false(has_system_sight(ctx.em, 2, foreign, false));

  // Add a foreign cruiser in planet orbit around Vega/Vega Prime (2, 1)
  const shipnum_t p2_cruiser = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                   .owned_by(foreign, 1)
                                   .in_planet_orbit(2, 1)
                                   .with_crew(50, 10)
                                   .build();
  test::expect_true(has_planet_sight(ctx.em, 2, 1, foreign, false));
  test::expect_false(has_planet_sight(ctx.em, 2, 1, viewer, false));

  // Verify is_ship_visible_to rules
  const Ship& s1 = *ctx.em.peek_ship(p1_cruiser);
  const Ship& s2 = *ctx.em.peek_ship(p2_cruiser);
  test::expect_true(is_ship_visible_to(s1, viewer, false, false));
  test::expect_false(is_ship_visible_to(s2, viewer, false, false));
  test::expect_true(is_ship_visible_to(s2, viewer, false, true));
  test::expect_true(is_ship_visible_to(s2, viewer, true, false));

  // Foreign space mines, canisters, greenhouse gases, and deep-space ships are
  // hidden from mortals even if has_sight_at_location is true, while landed
  // mines are visible when has_sight_at_location is true, and destroyed ships
  // (!alive()) are never visible.
  const shipnum_t p2_mine = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                                .owned_by(foreign, 1)
                                .in_star_orbit(2)
                                .build();
  const shipnum_t p2_landed_mine = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                                       .owned_by(foreign, 1)
                                       .landed_on(2, 1, {0, 0})
                                       .build();
  const shipnum_t p2_canist = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                                  .owned_by(foreign, 1)
                                  .in_planet_orbit(2, 1)
                                  .build();
  const shipnum_t p2_green = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                                 .owned_by(foreign, 1)
                                 .in_planet_orbit(2, 1)
                                 .build();
  const shipnum_t p2_deep = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                .owned_by(foreign, 1)
                                .in_deep_space()
                                .build();
  Ship dead_ship{};
  dead_ship.alive() = false;

  test::expect_false(
      is_ship_visible_to(*ctx.em.peek_ship(p2_mine), viewer, false, true));
  test::expect_true(
      is_ship_visible_to(*ctx.em.peek_ship(p2_mine), foreign, false, false));
  test::expect_true(is_ship_visible_to(*ctx.em.peek_ship(p2_landed_mine),
                                       viewer, false, true));
  test::expect_false(
      is_ship_visible_to(*ctx.em.peek_ship(p2_canist), viewer, false, true));
  test::expect_false(
      is_ship_visible_to(*ctx.em.peek_ship(p2_green), viewer, false, true));
  test::expect_false(
      is_ship_visible_to(*ctx.em.peek_ship(p2_deep), viewer, false, true));
  test::expect_true(
      is_ship_visible_to(*ctx.em.peek_ship(p2_deep), viewer, true, false));
  test::expect_false(is_ship_visible_to(dead_ship, viewer, true, true));
}

}  // namespace

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race with translation capability
  Race race{};
  race.Playernum = 1;
  race.translate[player_t{1}] = 100;  // Full translation for player 1
  race.translate[player_t{2}] = 50;   // Partial translation for player 2
  race.translate[player_t{3}] = 5;    // Very low translation for player 3

  RaceRepository races(store);
  races.save(race);

  const auto* saved = em.peek_race(1);
  test::expect_ne(saved, nullptr);

  // Test with integer values
  std::string result_int = saved->estimate(42, 1);
  test::expect_eq(result_int, "42");

  std::string result_int_k = saved->estimate(5000, 1);
  test::expect_true(result_int_k == "5.0K" || result_int_k == "5K");

  std::string result_int_m = saved->estimate(1500000, 1);
  test::expect_eq(result_int_m, "1.5M");

  // Test with negative integer
  std::string result_neg = saved->estimate(-42, 1);
  test::expect_eq(result_neg, "42");  // std::abs() is applied in the function

  // Test with double values
  std::string result_double = saved->estimate(42.7, 1);
  test::expect_eq(result_double, "42");

  std::string result_double_k = saved->estimate(5000.5, 1);
  test::expect_true(result_double_k == "5.0K" || result_double_k == "5K");

  std::string result_double_m = saved->estimate(1500000.0, 1);
  test::expect_eq(result_double_m, "1.5M");

  // Test edge case: exactly 1 million
  std::string result_1m = saved->estimate(1000000, 1);
  test::expect_eq(result_1m, "1.0M");

  // Test with low translation
  std::string result_low_trans = saved->estimate(42, 3);
  test::expect_eq(result_low_trans, "?");

  // Test with float type
  float float_val = 42.5f;
  std::string result_float = saved->estimate(float_val, 1);
  test::expect_eq(result_float, "42");

  // Test overload taking const Race&
  std::string result_race_ref = saved->estimate(42, *saved);
  test::expect_eq(result_race_ref, "42");

  // Test estimate_value quantization and threshold
  test::expect_eq(saved->estimate_value(149, 1),
                  std::optional<std::int64_t>{149});
  test::expect_eq(saved->estimate_value(149, *saved),
                  std::optional<std::int64_t>{149});
  // At 50% translation (player 2), k = 101 - 50 = 51 -> (149 / 51) * 51 = 102
  test::expect_eq(saved->estimate_value(149, 2),
                  std::optional<std::int64_t>{102});
  test::expect_eq(saved->estimate(149, 2), "102");
  test::expect_eq(saved->estimate(50000, 1), "50K");
  // At 5% translation (player 3, <= 10), estimate_value returns std::nullopt
  test::expect_false(saved->estimate_value(149, 3).has_value());

  // Floating-point estimate_value preserves fractional traits (mass,
  // metabolism, birthrate) without truncating to 0
  test::expect_eq(saved->estimate_value(0.75, 1), std::optional<double>{0.75});
  test::expect_eq(saved->estimate_value(149.75, *saved),
                  std::optional<double>{149.75});
  test::expect_eq(saved->estimate_value(0.75, 2), std::optional<double>{0.51});
  test::expect_eq(saved->estimate_value(149.5, 2),
                  std::optional<double>{102.0});
  test::expect_false(saved->estimate_value(0.75, 3).has_value());

  test_ship_visibility_helpers();

  std::println(std::cout, "All estimate tests passed!");
  return 0;
}
