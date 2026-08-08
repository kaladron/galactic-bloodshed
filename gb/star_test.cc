// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

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

    assert(star.get_name() == "Sol");
    assert(star.numplanets() == 3);
    assert(star.get_planet_name(0) == "Mercury");
    assert(star.get_planet_name(1) == "Venus");
    assert(star.get_planet_name(2) == "Earth");
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
    assert(star.get_planet_name(0) == "Planet1");
    assert(star.get_planet_name(1) == "Planet2");

    // Out of bounds - should throw exception
    bool caught_exception = false;
    try {
      (void)star.get_planet_name(2);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
      std::string msg = e.what();
      assert(msg.find("Planet number 2 out of range") != std::string::npos);
    }
    assert(caught_exception);
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

    assert(star.planet_name_isset(0) == true);   // Has name
    assert(star.planet_name_isset(1) == false);  // Empty name
    assert(star.planet_name_isset(2) == true);   // Has name

    // Out of bounds - should throw exception
    bool caught_exception = false;
    try {
      (void)star.planet_name_isset(99);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);
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
    std::println(std::cout, "  ✓ Auto-resize works correctly");
  }

  // Overwriting existing planet names
  std::println(std::cout, "Overwriting existing planet names...");
  {
    star_struct s{};
    s.name = "Test";
    s.pnames.push_back("OldName");

    Star star(s);
    assert(star.get_planet_name(0) == "OldName");

    star.set_planet_name(0, "NewName");
    assert(star.get_planet_name(0) == "NewName");
    assert(star.numplanets() == 1);  // Size unchanged
    std::println(std::cout, "  ✓ Overwriting works correctly");
  }

  // Empty star (no planets, bounds checking throws)
  std::println(std::cout, "Empty star (no planets)...");
  {
    star_struct s{};
    s.name = "EmptyStar";
    // Don't add any planets

    Star star(s);
    assert(star.numplanets() == 0);

    // Out of bounds access should throw
    bool caught_exception = false;
    try {
      (void)star.get_planet_name(0);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);

    // planet_name_isset should also throw
    caught_exception = false;
    try {
      (void)star.planet_name_isset(0);
    } catch (const std::runtime_error& e) {
      caught_exception = true;
    }
    assert(caught_exception);

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
    std::println(std::cout, "  ✓ numplanets() correctly reflects vector size");
  }

  std::println(std::cout, "\n✓ All Star class tests passed!");
  return 0;
}
