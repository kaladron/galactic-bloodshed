// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

int main() {
  std::println("=== Testing Coordinates ===");

  // Default constructor
  Coordinates c0{};
  assert(c0.x == 0 && c0.y == 0);

  // Parameter constructor
  Coordinates c1{5, 10};
  assert(c1.x == 5 && c1.y == 10);

  // Arithmetic operators
  Coordinates c2{2, 3};
  Coordinates c_sum = c1 + c2;
  assert(c_sum.x == 7 && c_sum.y == 13);

  Coordinates c_diff = c1 - c2;
  assert(c_diff.x == 3 && c_diff.y == 7);

  Coordinates c_compound{1, 1};
  c_compound += c2;
  assert(c_compound.x == 3 && c_compound.y == 4);

  c_compound -= c2;
  assert(c_compound.x == 1 && c_compound.y == 1);

  // Comparisons
  assert(c1 == Coordinates(5, 10));
  assert(c1 != c2);
  assert(c2 < c1);

  // Parsing valid strings
  auto p1 = Coordinates::parse("5,10");
  assert(p1.has_value());
  assert(p1->x == 5 && p1->y == 10);

  auto p2 = Coordinates::parse("  12 , -34  ");
  assert(p2.has_value());
  assert(p2->x == 12 && p2->y == -34);

  auto p3 = Coordinates::parse("0,0");
  assert(p3.has_value());
  assert(p3->x == 0 && p3->y == 0);

  // Parsing invalid strings
  assert(!Coordinates::parse("").has_value());
  assert(!Coordinates::parse("5").has_value());
  assert(!Coordinates::parse("5,").has_value());
  assert(!Coordinates::parse(",10").has_value());
  assert(!Coordinates::parse("abc,10").has_value());
  assert(!Coordinates::parse("5,xyz").has_value());
  assert(!Coordinates::parse("5 10").has_value());

  // Formatting with std::format
  std::string formatted = std::format("{}", c1);
  assert(formatted == "5,10");

  std::println("✓ All Coordinates unit tests passed!");
  return 0;
}
