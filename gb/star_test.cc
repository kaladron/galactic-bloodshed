// SPDX-License-Identifier: Apache-2.0

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

  // Test 2: Bounds checking on get_planet_name (out of range returns empty string)
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
    
    // Out of bounds - should return empty string
    assert(star.get_planet_name(2) == "");
    assert(star.get_planet_name(99) == "");
    std::println("  ✓ Out of bounds access returns empty string");
  }

  // Test 3: planet_name_isset bounds checking
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
    assert(star.planet_name_isset(99) == false); // Out of bounds
    std::println("  ✓ planet_name_isset works correctly");
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

  // Test 6: Empty star (no planets)
  std::println("Test 6: Empty star (no planets)...");
  {
    star_struct s{};
    s.name = "EmptyStar";
    // Don't add any planets
    
    Star star(s);
    assert(star.numplanets() == 0);
    assert(star.get_planet_name(0) == "");
    assert(star.planet_name_isset(0) == false);
    std::println("  ✓ Empty star works correctly");
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
