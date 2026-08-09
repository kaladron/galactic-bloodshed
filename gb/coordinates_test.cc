// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

int main() {
  std::println(std::cout, "=== Testing Coordinates & API Integration ===");

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

  // --- Planet is_valid & wrap tests ---
  planet_struct pdata{};
  pdata.Maxx = 10;
  pdata.Maxy = 8;
  Planet planet(pdata);

  assert(planet.is_valid({0, 0}));
  assert(planet.is_valid({9, 7}));
  assert(!planet.is_valid({-1, 0}));
  assert(!planet.is_valid({10, 5}));
  assert(!planet.is_valid({5, 8}));

  // Toroidal wrapping test
  assert(planet.wrap({10, 3}) == Coordinates(0, 3));
  assert(planet.wrap({-1, 3}) == Coordinates(9, 3));
  assert(planet.wrap({15, 3}) == Coordinates(5, 3));

  // --- Sector & SectorMap tests ---
  SectorMap smap(planet, true);  // Initialize empty grid (10x8 = 80 sectors)
  assert(smap.in_bounds({5, 5}));
  assert(!smap.in_bounds({10, 5}));

  Coordinates target_c{3, 4};
  auto& sect = smap.get(target_c);
  assert(sect.get_x() == 0 &&
         sect.get_y() == 0);  // Initially default-constructed

  // Setting and checking coords
  sector_struct s_data{};
  s_data.coords = {3, 4};
  s_data.eff = 85;
  smap.set(target_c, s_data);

  const auto& const_smap = smap;
  const auto& fetched = const_smap.get(target_c);
  assert(fetched.coords() == target_c);
  assert(fetched.get_eff() == 85);

  // Range view: smap.coordinates()
  int coord_count = 0;
  for (Coordinates c : smap.coordinates()) {
    assert(smap.in_bounds(c));
    coord_count++;
  }
  assert(coord_count == 80);

  // Range view: smap.indexed_sectors()
  int indexed_count = 0;
  for (auto [c, s] : smap.indexed_sectors()) {
    assert(smap.in_bounds(c));
    indexed_count++;
  }
  assert(indexed_count == 80);

  // --- Ship land_coords tests ---
  ship_struct shipdata{};
  Ship ship(shipdata);
  ship.set_land_coords({7, 2});
  assert(ship.land_coords() == Coordinates(7, 2));
  assert(ship.land_x() == 7 && ship.land_y() == 2);

  std::println(std::cout, "✓ All Coordinates & API Integration tests passed!");
  return 0;
}
