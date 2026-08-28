// SPDX-License-Identifier: Apache-2.0

/// \file star_test.cc
/// \brief Unit tests for Star class methods, planet name manipulation,
/// auto-resizing, and bounds checking.

import dallib;
import gb.entities;
import test;
import std;

int main() {
  // Basic star creation with vector of planet names
  std::println(std::cout, "Basic star creation with planet names...");
  {
    star_struct s{};
    s.name = "Sol";
    s.pnames.push_back("Mercury");
    s.pnames.push_back("Venus");
    s.pnames.push_back("Earth");

    Star star(s);

    test::expect_eq(star.get_name(), "Sol");
    test::expect_eq(star.numplanets(), 3);
    test::expect_eq(star.get_planet_name(0), "Mercury");
    test::expect_eq(star.get_planet_name(1), "Venus");
    test::expect_eq(star.get_planet_name(2), "Earth");
    std::println(std::cout, "  ✓ Basic creation and access works");
  }

  // Bounds checking on get_planet_name (out of range throws exception)
  std::println(std::cout, "Bounds checking on get_planet_name...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet1");
    s.pnames.push_back("Planet2");

    Star star(s);

    // Valid access
    test::expect_eq(star.get_planet_name(0), "Planet1");
    test::expect_eq(star.get_planet_name(1), "Planet2");

    // Out of bounds - should throw exception
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.get_planet_name(2); });
    std::println(std::cout, "  ✓ Out of bounds access throws exception");
  }

  // planet_name_isset bounds checking (throws on out of bounds)
  std::println(std::cout, "planet_name_isset bounds checking...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet1");
    s.pnames.push_back("");  // Empty name
    s.pnames.push_back("Planet3");

    Star star(s);

    test::expect_true(star.planet_name_isset(0));   // Has name
    test::expect_false(star.planet_name_isset(1));  // Empty name
    test::expect_true(star.planet_name_isset(2));   // Has name

    // Out of bounds - should throw exception
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.planet_name_isset(99); });
    std::println(
        std::cout,
        "  ✓ planet_name_isset works correctly and throws on out of bounds");
  }

  // set_planet_name with auto-resize
  std::println(std::cout, "set_planet_name with auto-resize...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet0");

    Star star(s);
    test::expect_eq(star.numplanets(), 1);

    // Set planet at index 5 - should auto-resize vector
    star.set_planet_name(5, "Jupiter");
    test::expect_eq(star.numplanets(), 6);

    // Check that intermediate planets exist but are empty
    test::expect_eq(star.get_planet_name(0), "Planet0");
    test::expect_eq(star.get_planet_name(1), "");
    test::expect_eq(star.get_planet_name(2), "");
    test::expect_eq(star.get_planet_name(3), "");
    test::expect_eq(star.get_planet_name(4), "");
    test::expect_eq(star.get_planet_name(5), "Jupiter");
    std::println(std::cout, "  ✓ Auto-resize works correctly");
  }

  // Overwriting existing planet names
  std::println(std::cout, "Overwriting existing planet names...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("OldName");

    Star star(s);
    test::expect_eq(star.get_planet_name(0), "OldName");

    star.set_planet_name(0, "NewName");
    test::expect_eq(star.get_planet_name(0), "NewName");
    test::expect_eq(star.numplanets(), 1);  // Size unchanged
    std::println(std::cout, "  ✓ Overwriting works correctly");
  }

  // Empty star (no planets, bounds checking throws)
  std::println(std::cout, "Empty star (no planets)...");
  {
    star_struct s{};
    s.name = "EmptyStar";
    // Don't add any planets

    Star star(s);
    test::expect_eq(star.numplanets(), 0);

    // Out of bounds access should throw
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.get_planet_name(0); });

    // planet_name_isset should also throw
    test::expect_throws<std::runtime_error>(
        [&]() { (void)star.planet_name_isset(0); });

    std::println(
        std::cout,
        "  ✓ Empty star works correctly with exception-based bounds checking");
  }

  // numplanets() reflects vector size
  std::println(std::cout, "numplanets() reflects vector size...");
  {
    star_struct s{};
    s.name = "Test";

    Star star(s);
    test::expect_eq(star.numplanets(), 0);

    // Modify through struct (simulating direct construction)
    s.pnames.push_back("P1");
    s.pnames.push_back("P2");
    s.pnames.push_back("P3");
    Star star2(s);
    test::expect_eq(star2.numplanets(), 3);

    // Modify through Star interface
    star2.set_planet_name(3, "P4");
    test::expect_eq(star2.numplanets(), 4);
    std::println(std::cout, "  ✓ numplanets() correctly reflects vector size");
  }

  std::println(std::cout, "\n✓ All Star class tests passed!");
  return 0;
}
