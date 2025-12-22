// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // Create a test planet with known dimensions
  Planet planet(PlanetType::EARTH);
  planet.Maxx() = 10;  // 0-9 range for x-coordinates
  planet.Maxy() = 8;   // 0-7 range for y-coordinates

  // Test numeric direction mappings (1-9, excluding 5)

  // Direction '1' (southwest): x-1, y+1 with x-wrapping
  {
    auto result = get_move(planet, '1', {5, 3});
    assert(result.x == 4 && result.y == 4);

    // Test x-wrapping at left boundary
    result = get_move(planet, '1', {0, 3});
    assert(result.x == 9 && result.y == 4);  // wraps to Maxx-1
  }

  // Direction '2' (south): x unchanged, y+1
  {
    auto result = get_move(planet, '2', {5, 3});
    assert(result.x == 5 && result.y == 4);
  }

  // Direction '3' (southeast): x+1, y+1 with x-wrapping
  {
    auto result = get_move(planet, '3', {5, 3});
    assert(result.x == 6 && result.y == 4);

    // Test x-wrapping at right boundary
    result = get_move(planet, '3', {9, 3});
    assert(result.x == 0 && result.y == 4);  // wraps to 0
  }

  // Direction '4' (west): x-1, y unchanged with x-wrapping
  {
    auto result = get_move(planet, '4', {5, 3});
    assert(result.x == 4 && result.y == 3);

    // Test x-wrapping at left boundary
    result = get_move(planet, '4', {0, 3});
    assert(result.x == 9 && result.y == 3);  // wraps to Maxx-1
  }

  // Direction '6' (east): x+1, y unchanged with x-wrapping
  {
    auto result = get_move(planet, '6', {5, 3});
    assert(result.x == 6 && result.y == 3);

    // Test x-wrapping at right boundary
    result = get_move(planet, '6', {9, 3});
    assert(result.x == 0 && result.y == 3);  // wraps to 0
  }

  // Direction '7' (northwest): x-1, y-1 with x-wrapping
  {
    auto result = get_move(planet, '7', {5, 3});
    assert(result.x == 4 && result.y == 2);

    // Test x-wrapping at left boundary
    result = get_move(planet, '7', {0, 3});
    assert(result.x == 9 && result.y == 2);  // wraps to Maxx-1
  }

  // Direction '8' (north): x unchanged, y-1
  {
    auto result = get_move(planet, '8', {5, 3});
    assert(result.x == 5 && result.y == 2);
  }

  // Direction '9' (northeast): x+1, y-1 with x-wrapping
  {
    auto result = get_move(planet, '9', {5, 3});
    assert(result.x == 6 && result.y == 2);

    // Test x-wrapping at right boundary
    result = get_move(planet, '9', {9, 3});
    assert(result.x == 0 && result.y == 2);  // wraps to 0
  }

  // Test letter direction mappings (vi-like movement keys)

  // 'b' maps to '1' (southwest)
  {
    auto result = get_move(planet, 'b', {5, 3});
    assert(result.x == 4 && result.y == 4);
  }

  // 'k' maps to '2' (south)
  {
    auto result = get_move(planet, 'k', {5, 3});
    assert(result.x == 5 && result.y == 4);
  }

  // 'n' maps to '3' (southeast)
  {
    auto result = get_move(planet, 'n', {5, 3});
    assert(result.x == 6 && result.y == 4);
  }

  // 'h' maps to '4' (west)
  {
    auto result = get_move(planet, 'h', {5, 3});
    assert(result.x == 4 && result.y == 3);
  }

  // 'l' maps to '6' (east)
  {
    auto result = get_move(planet, 'l', {5, 3});
    assert(result.x == 6 && result.y == 3);
  }

  // 'y' maps to '7' (northwest)
  {
    auto result = get_move(planet, 'y', {5, 3});
    assert(result.x == 4 && result.y == 2);
  }

  // 'j' maps to '8' (north)
  {
    auto result = get_move(planet, 'j', {5, 3});
    assert(result.x == 5 && result.y == 2);
  }

  // 'u' maps to '9' (northeast)
  {
    auto result = get_move(planet, 'u', {5, 3});
    assert(result.x == 6 && result.y == 2);
  }

  // Test boundary conditions for y-coordinates (no wrapping)

  // Moving south from bottom edge
  {
    auto result = get_move(planet, '2', {5, 7});  // Maxy-1
    assert(result.x == 5 && result.y == 8);       // Can go beyond Maxy
  }

  // Moving north from top edge
  {
    auto result = get_move(planet, '8', {5, 0});
    assert(result.x == 5 && result.y == -1);  // Can go below 0
  }

  // Test edge cases with different planet sizes

  // Test with minimal planet size
  Planet small_planet(PlanetType::ASTEROID);
  small_planet.Maxx() = 2;  // 0-1 range
  small_planet.Maxy() = 3;  // 0-2 range

  {
    // Test wrapping on small planet
    auto result = get_move(small_planet, '6', {1, 1});  // east from x=1
    assert(result.x == 0 && result.y == 1);             // wraps to 0

    result = get_move(small_planet, '4', {0, 1});  // west from x=0
    assert(result.x == 1 && result.y == 1);        // wraps to Maxx-1
  }

  // Test invalid directions (should return original coordinates)
  {
    auto result =
        get_move(planet, '5', {5, 3});       // '5' is not a valid direction
    assert(result.x == 5 && result.y == 3);  // unchanged

    result = get_move(planet, 'z', {5, 3});  // 'z' is not a valid direction
    assert(result.x == 5 && result.y == 3);  // unchanged

    result = get_move(planet, '0', {5, 3});  // '0' is not a valid direction
    assert(result.x == 5 && result.y == 3);  // unchanged
  }

  // Test at exact boundary conditions
  {
    // Test at planet.Maxx-1 boundary (rightmost valid position)
    auto result = get_move(planet, '6', {9, 3});  // x == Maxx-1, moving east
    assert(result.x == 0 && result.y == 3);       // wraps to 0

    // Test at x == 0 boundary (leftmost position)
    result = get_move(planet, '4', {0, 3});  // west from x=0
    assert(result.x == 9 && result.y == 3);  // wraps to Maxx-1
  }

  // Test coordinates preservation in Coordinates struct
  {
    Coordinates start{7, 2};
    auto result = get_move(planet, '6', start);
    assert(result.x == 8 && result.y == 2);
    // Verify original coordinates unchanged
    assert(start.x == 7 && start.y == 2);
  }

  return 0;
}
