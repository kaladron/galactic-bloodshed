// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Test 1: Basic star creation with vector of planet names
  std::println("Test 1: Basic star creation with planet names...");
  {
    star_struct s{};
    s.name = "Sol";
    s.pnames.push_back("Mercury");
    s.pnames.push_back("Venus");
    s.pnames.push_back("Earth");

    Star star(s);

    assert(star.get_name() == "Sol");
    assert(star.numplanets() == 3);
    assert(star.get_planet_name(0) == "Mercury");
    assert(star.get_planet_name(1) == "Venus");
    assert(star.get_planet_name(2) == "Earth");
    std::println("  ✓ Basic creation and access works");
  }

  // Test 2: Bounds checking on get_planet_name (out of range throws exception)
  std::println("Test 2: Bounds checking on get_planet_name...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet1");
    s.pnames.push_back("Planet2");

    Star star(s);

    // Valid access
    assert(star.get_planet_name(0) == "Planet1");
    assert(star.get_planet_name(1) == "Planet2");

    // Out of bounds - should throw exception
    bool caught_exception = false;
    try {
      star.get_planet_name(2);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
      std::string msg = e.what();
      assert(msg.find("Planet number 2 out of range") != std::string::npos);
    }
    assert(caught_exception);
    std::println("  ✓ Out of bounds access throws exception");
  }

  // Test 3: planet_name_isset bounds checking (throws on out of bounds)
  std::println("Test 3: planet_name_isset bounds checking...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet1");
    s.pnames.push_back("");  // Empty name
    s.pnames.push_back("Planet3");

    Star star(s);

    assert(star.planet_name_isset(0) == true);   // Has name
    assert(star.planet_name_isset(1) == false);  // Empty name
    assert(star.planet_name_isset(2) == true);   // Has name

    // Out of bounds - should throw exception
    bool caught_exception = false;
    try {
      star.planet_name_isset(99);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);
    std::println(
        "  ✓ planet_name_isset works correctly and throws on out of bounds");
  }

  // Test 4: set_planet_name with auto-resize
  std::println("Test 4: set_planet_name with auto-resize...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("Planet0");

    Star star(s);
    assert(star.numplanets() == 1);

    // Set planet at index 5 - should auto-resize vector
    star.set_planet_name(5, "Jupiter");
    assert(star.numplanets() == 6);

    // Check that intermediate planets exist but are empty
    assert(star.get_planet_name(0) == "Planet0");
    assert(star.get_planet_name(1) == "");
    assert(star.get_planet_name(2) == "");
    assert(star.get_planet_name(3) == "");
    assert(star.get_planet_name(4) == "");
    assert(star.get_planet_name(5) == "Jupiter");
    std::println("  ✓ Auto-resize works correctly");
  }

  // Test 5: Overwriting existing planet names
  std::println("Test 5: Overwriting existing planet names...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("OldName");

    Star star(s);
    assert(star.get_planet_name(0) == "OldName");

    star.set_planet_name(0, "NewName");
    assert(star.get_planet_name(0) == "NewName");
    assert(star.numplanets() == 1);  // Size unchanged
    std::println("  ✓ Overwriting works correctly");
  }

  // Test 6: Empty star (no planets, bounds checking throws)
  std::println("Test 6: Empty star (no planets)...");
  {
    star_struct s{};
    s.name = "EmptyStar";
    // Don't add any planets

    Star star(s);
    assert(star.numplanets() == 0);

    // Out of bounds access should throw
    bool caught_exception = false;
    try {
      star.get_planet_name(0);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);

    // planet_name_isset should also throw
    caught_exception = false;
    try {
      star.planet_name_isset(0);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);

    std::println(
        "  ✓ Empty star works correctly with exception-based bounds checking");
  }

  // Test 7: numplanets() reflects vector size
  std::println("Test 7: numplanets() reflects vector size...");
  {
    star_struct s{};
    s.name = "Test";

    Star star(s);
    assert(star.numplanets() == 0);

    // Modify through struct (simulating direct construction)
    s.pnames.push_back("P1");
    s.pnames.push_back("P2");
    s.pnames.push_back("P3");
    Star star2(s);
    assert(star2.numplanets() == 3);

    // Modify through Star interface
    star2.set_planet_name(3, "P4");
    assert(star2.numplanets() == 4);
    std::println("  ✓ numplanets() correctly reflects vector size");
  }

  std::println("\n✓ All Star class tests passed!");
  return 0;
}
