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

  std::println("Planet unit tests passed successfully!");
  return 0;
}
