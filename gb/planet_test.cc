// SPDX-License-Identifier: Apache-2.0

/// \file planet_test.cc
/// \brief Unit tests for Planet domain methods, dimensions, and toroidal
/// geometry.

import gblib;
import test;
import std;

int main() {
  // Test 1: Planet default and mutable dimensions
  {
    Planet planet(PlanetType::EARTH);
    test::expect_eq(planet.dimensions(), Coordinates(0, 0));
    test::expect_eq(planet.Maxx(), 0);
    test::expect_eq(planet.Maxy(), 0);

    planet.dimensions() = Coordinates(20, 10);
    test::expect_eq(planet.dimensions().x, 20);
    test::expect_eq(planet.dimensions().y, 10);
    test::expect_eq(planet.Maxx(), 20);
    test::expect_eq(planet.Maxy(), 10);

    // Forwarder assignment compatibility
    planet.Maxx() = 12;
    planet.Maxy() = 6;
    test::expect_eq(planet.dimensions(), Coordinates(12, 6));
  }

  // Test 2: Bounds checking with is_valid
  {
    Planet planet(PlanetType::EARTH);
    planet.dimensions() = Coordinates(10, 8);

    test::expect_true(planet.is_valid({0, 0}));
    test::expect_true(planet.is_valid({9, 7}));
    test::expect_true(planet.is_valid({5, 4}));

    test::expect_false(planet.is_valid({-1, 0}));
    test::expect_false(planet.is_valid({0, -1}));
    test::expect_false(planet.is_valid({10, 0}));
    test::expect_false(planet.is_valid({0, 8}));
    test::expect_false(planet.is_valid({10, 8}));
  }

  // Test 3: Toroidal coordinate wrapping
  {
    Planet planet(PlanetType::EARTH);
    planet.dimensions() = Coordinates(10, 8);

    // Within bounds: unchanged
    test::expect_eq(planet.wrap({0, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({9, 3}), Coordinates(9, 3));

    // Horizontal wrap across right boundary
    test::expect_eq(planet.wrap({10, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({15, 3}), Coordinates(5, 3));

    // Horizontal wrap across left boundary
    test::expect_eq(planet.wrap({-1, 3}), Coordinates(9, 3));
    test::expect_eq(planet.wrap({-10, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({-11, 3}), Coordinates(9, 3));

    // Zero width safety check
    Planet uninit_planet(PlanetType::EARTH);
    test::expect_eq(uninit_planet.wrap({5, 3}), Coordinates(5, 3));
  }

  // Test 4: Planet gravity calculation
  {
    Planet planet(PlanetType::EARTH);
    planet.dimensions() = Coordinates(20, 10);
    const double expected_gravity = 20.0 * 10.0 * GRAV_FACTOR;
    test::expect_true(std::abs(planet.gravity() - expected_gravity) < 1e-6);
  }

  // Test 5: Coordinate adjacency on planet surface
  {
    Planet planet(PlanetType::EARTH);
    planet.dimensions() = Coordinates(10, 8);

    // Direct and diagonal neighbors
    test::expect_true(adjacent(planet, {5, 5}, {5, 5}));
    test::expect_true(adjacent(planet, {5, 5}, {6, 5}));
    test::expect_true(adjacent(planet, {5, 5}, {4, 5}));
    test::expect_true(adjacent(planet, {5, 5}, {5, 6}));
    test::expect_true(adjacent(planet, {5, 5}, {5, 4}));
    test::expect_true(adjacent(planet, {5, 5}, {6, 6}));
    test::expect_true(adjacent(planet, {5, 5}, {4, 4}));

    // Non-adjacent locations
    test::expect_false(adjacent(planet, {5, 5}, {7, 5}));
    test::expect_false(adjacent(planet, {5, 5}, {5, 7}));
    test::expect_false(adjacent(planet, {5, 5}, {7, 7}));

    // Toroidal seam wrapping adjacency
    test::expect_true(adjacent(planet, {0, 4}, {9, 4}));
    test::expect_true(adjacent(planet, {0, 4}, {9, 5}));
    test::expect_true(adjacent(planet, {0, 4}, {9, 3}));
    test::expect_true(adjacent(planet, {9, 4}, {0, 4}));

    // Non-wrapping vertical separation across polar caps
    test::expect_false(adjacent(planet, {5, 0}, {5, 7}));
  }

  // Test 6: Planet compatibility with race conditions
  {
    Planet planet(PlanetType::EARTH);
    for (int i = 0; i <= TOXIC; ++i) {
      planet.conditions(static_cast<Conditions>(i)) = 50;
    }
    planet.conditions(TEMP) = 100;
    planet.conditions(TOXIC) = 0;

    Race race{};
    for (int i = 0; i <= OTHER; ++i) {
      race.conditions[i] = 50;
    }
    race.conditions[TEMP] = 100;

    const double compat = planet.compatibility(race);
    test::expect_gt(compat, 90.0);
  }

  // Test 7: plinfo defaults and optional tox_thresh behavior
  {
    Planet planet(PlanetType::EARTH);
    auto& info = planet.info(player_t{1});

    // Verify initial default states
    test::expect_eq(info.tox_thresh, std::nullopt);
    test::expect_false(info.explored);
    test::expect_eq(info.autorep, 0U);
    test::expect_eq(info.tax, 0U);
    test::expect_eq(info.newtax, 0U);
    test::expect_eq(info.comread, 0U);
    test::expect_eq(info.mob_set, 0U);
    test::expect_eq(info.guns, 0U);

    // Verify mutating tox_thresh with value and resetting to nullopt
    info.tox_thresh = 35U;
    test::expect_true(info.tox_thresh.has_value());
    test::expect_eq(info.tox_thresh.value(), 35U);
    test::expect_eq(info.tox_thresh.value_or(0U), 35U);

    info.tox_thresh = std::nullopt;
    test::expect_false(info.tox_thresh.has_value());
    test::expect_eq(info.tox_thresh.value_or(0U), 0U);

    // Verify 32-bit unsigned fields
    info.tax = 45U;
    info.newtax = 50U;
    info.comread = 85U;
    info.mob_set = 95U;
    info.guns = 19U;
    info.autorep = 63U;
    info.explored = true;

    // Verify 64-bit resource_t stockpile and production fields
    info.resource = 1'000'000'000LL;
    info.fuel = 500'000'000LL;
    info.destruct = 250'000'000LL;
    info.crystals = 100'000'000LL;
    info.prod_res = 100'000LL;
    info.prod_fuel = 80'000LL;
    info.prod_dest = 70'000LL;
    info.prod_crystals = 60'000LL;
    info.numsectsowned = 500;

    test::expect_eq(info.resource, 1'000'000'000LL);
    test::expect_eq(info.fuel, 500'000'000LL);
    test::expect_eq(info.destruct, 250'000'000LL);
    test::expect_eq(info.crystals, 100'000'000LL);
    test::expect_eq(info.prod_res, 100'000LL);
    test::expect_eq(info.prod_fuel, 80'000LL);
    test::expect_eq(info.prod_dest, 70'000LL);
    test::expect_eq(info.prod_crystals, 60'000LL);
    test::expect_eq(info.numsectsowned, 500U);
  }

  // Test 8: Planet exploration timer and explored flag
  {
    Planet planet(PlanetType::EARTH);
    test::expect_eq(planet.expltimer(), 0U);
    test::expect_false(planet.explored());

    planet.expltimer() = 5U;
    planet.explored() = true;

    test::expect_eq(planet.expltimer(), 5U);
    test::expect_true(planet.explored());
  }

  std::println("Planet unit tests passed successfully!");
  return 0;
}
