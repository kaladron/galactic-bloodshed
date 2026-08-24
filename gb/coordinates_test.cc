// SPDX-License-Identifier: Apache-2.0

/// \file coordinates_test.cc
/// \brief Unit tests for Coordinates struct, arithmetic, parsing, formatting,
/// and SectorMap integration.

import gblib;
import test;
import std;

int main() {
  std::println(std::cout, "=== Testing Coordinates & API Integration ===");

  // Default constructor
  Coordinates c0{};
  test::expect_eq(c0.x, 0);
  test::expect_eq(c0.y, 0);

  // Parameter constructor
  Coordinates c1{5, 10};
  test::expect_eq(c1.x, 5);
  test::expect_eq(c1.y, 10);

  // Arithmetic operators
  Coordinates c2{2, 3};
  Coordinates c_sum = c1 + c2;
  test::expect_eq(c_sum.x, 7);
  test::expect_eq(c_sum.y, 13);

  Coordinates c_diff = c1 - c2;
  test::expect_eq(c_diff.x, 3);
  test::expect_eq(c_diff.y, 7);

  Coordinates c_compound{1, 1};
  c_compound += c2;
  test::expect_eq(c_compound.x, 3);
  test::expect_eq(c_compound.y, 4);

  c_compound -= c2;
  test::expect_eq(c_compound.x, 1);
  test::expect_eq(c_compound.y, 1);

  // Comparisons
  test::expect_eq(c1, Coordinates(5, 10));
  test::expect_ne(c1, c2);
  test::expect_lt(c2, c1);

  // Parsing valid strings
  auto p1 = Coordinates::parse("5,10");
  test::expect_true(p1.has_value());
  test::expect_eq(p1->x, 5);
  test::expect_eq(p1->y, 10);

  auto p2 = Coordinates::parse("  12 , -34  ");
  test::expect_true(p2.has_value());
  test::expect_eq(p2->x, 12);
  test::expect_eq(p2->y, -34);

  auto p3 = Coordinates::parse("0,0");
  test::expect_true(p3.has_value());
  test::expect_eq(p3->x, 0);
  test::expect_eq(p3->y, 0);

  // Parsing invalid strings
  test::expect_false(Coordinates::parse("").has_value());
  test::expect_false(Coordinates::parse("5").has_value());
  test::expect_false(Coordinates::parse("5,").has_value());
  test::expect_false(Coordinates::parse(",10").has_value());
  test::expect_false(Coordinates::parse("abc,10").has_value());
  test::expect_false(Coordinates::parse("5,xyz").has_value());
  test::expect_false(Coordinates::parse("5 10").has_value());

  // Formatting with std::format
  std::string formatted = std::format("{}", c1);
  test::expect_eq(formatted, "5,10");

  // --- Planet is_valid & wrap tests ---
  planet_struct pdata{};
  pdata.Maxx = 10;
  pdata.Maxy = 8;
  Planet planet(pdata);

  test::expect_true(planet.is_valid({0, 0}));
  test::expect_true(planet.is_valid({9, 7}));
  test::expect_false(planet.is_valid({-1, 0}));
  test::expect_false(planet.is_valid({10, 5}));
  test::expect_false(planet.is_valid({5, 8}));

  // Toroidal wrapping test
  test::expect_eq(planet.wrap({10, 3}), Coordinates(0, 3));
  test::expect_eq(planet.wrap({-1, 3}), Coordinates(9, 3));
  test::expect_eq(planet.wrap({15, 3}), Coordinates(5, 3));

  // --- Sector & SectorMap tests ---
  SectorMap smap(planet, true);  // Initialize empty grid (10x8 = 80 sectors)
  test::expect_true(smap.in_bounds({5, 5}));
  test::expect_false(smap.in_bounds({10, 5}));

  Coordinates target_c{3, 4};
  auto& sect = smap.get(target_c);
  test::expect_eq(sect.get_x(), 0);
  test::expect_eq(sect.get_y(), 0);  // Initially default-constructed

  // Setting and checking coords
  sector_struct s_data{};
  s_data.coords = {3, 4};
  s_data.eff = 85;
  smap.set(target_c, s_data);

  const auto& const_smap = smap;
  const auto& fetched = const_smap.get(target_c);
  test::expect_eq(fetched.coords(), target_c);
  test::expect_eq(fetched.get_eff(), 85);

  // Range view: smap.coordinates()
  int coord_count = 0;
  for (Coordinates c : smap.coordinates()) {
    test::expect_true(smap.in_bounds(c));
    coord_count++;
  }
  test::expect_eq(coord_count, 80);

  // Range view: smap.indexed_sectors()
  int indexed_count = 0;
  for (auto [c, s] : smap.indexed_sectors()) {
    test::expect_true(smap.in_bounds(c));
    indexed_count++;
  }
  test::expect_eq(indexed_count, 80);

  // --- Ship land_coords tests ---
  ship_struct shipdata{};
  Ship ship(shipdata);
  ship.set_land_coords({7, 2});
  test::expect_eq(ship.land_coords(), Coordinates(7, 2));
  test::expect_eq(ship.land_coords().x, 7);
  test::expect_eq(ship.land_coords().y, 2);

  std::println(std::cout, "✓ All Coordinates & API Integration tests passed!");
  return 0;
}
