// SPDX-License-Identifier: Apache-2.0

/// \file coordinates_test.cc
/// \brief Unit tests for Coordinates struct, arithmetic, parsing, formatting,
/// and SectorMap integration.

import gb.entities;
import test;
import std;

namespace {
// Floating-point comparison tolerance for trigonometry (std::atan2) and
// Euclidean distance calculations.
constexpr double floating_point_tolerance = 1e-5;

void expect_near(double actual, double expected,
                 double tolerance = floating_point_tolerance,
                 std::source_location loc = std::source_location::current()) {
  if (std::abs(actual - expected) > tolerance) {
    std::println(std::cerr,
                 "\n❌ [ASSERTION FAILED] {}:{}\n"
                 "    Expected ~: {}\n"
                 "    Actual:     {}\n"
                 "    Difference: {} (exceeds tolerance {})",
                 loc.file_name(), loc.line(), expected, actual,
                 std::abs(actual - expected), tolerance);
    test::expect_true(false, "Values not within tolerance", loc);
  }
}
}  // namespace

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
  pdata.dimensions = {10, 8};
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
  SectorMap smap(planet);  // Initialize empty grid (10x8 = 80 sectors)
  test::expect_true(smap.in_bounds(Coordinates{5, 5}));
  test::expect_false(smap.in_bounds(Coordinates{10, 5}));

  Coordinates target_c{3, 4};
  auto& sect = smap.get(target_c);
  test::expect_eq(sect.coords(),
                  target_c);  // Initialized by SectorMap constructor

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

  // Out of bounds checking with descriptive error messages
  try {
    [[maybe_unused]] auto& out = smap.get(Coordinates{10, 5});
    test::expect_true(false);  // Should not reach
  } catch (const std::out_of_range& e) {
    test::expect_eq(
        std::string(e.what()),
        "SectorMap::get(10, 5) out of bounds for dimensions (10, 8)");
  }

  try {
    smap.set(Coordinates{-1, 0}, s_data);
    test::expect_true(false);  // Should not reach
  } catch (const std::out_of_range& e) {
    test::expect_eq(
        std::string(e.what()),
        "SectorMap::set(-1, 0) out of bounds for dimensions (10, 8)");
  }

  // --- Ship land_coords tests ---
  ship_struct shipdata{};
  Ship ship(shipdata);
  ship.set_land_coords({7, 2});
  test::expect_eq(ship.land_coords(), Coordinates(7, 2));
  test::expect_eq(ship.land_coords().x, 7);
  test::expect_eq(ship.land_coords().y, 2);

  // --- SystemCoordinates tests ---
  std::println(std::cout, "--- Testing SystemCoordinates ---");
  {
    SystemCoordinates s0{};
    test::expect_eq(s0.x, 0.0);
    test::expect_eq(s0.y, 0.0);

    SystemCoordinates s1{10.5, -20.5};
    test::expect_eq(s1.x, 10.5);
    test::expect_eq(s1.y, -20.5);

    SystemCoordinates s2{2.5, 5.5};
    SystemCoordinates s_sum = s1 + s2;
    expect_near(s_sum.x, 13.0);
    expect_near(s_sum.y, -15.0);

    SystemCoordinates s_diff = s1 - s2;
    expect_near(s_diff.x, 8.0);
    expect_near(s_diff.y, -26.0);

    SystemCoordinates s_compound{1.0, 2.0};
    s_compound += s2;
    expect_near(s_compound.x, 3.5);
    expect_near(s_compound.y, 7.5);
    s_compound -= s2;
    expect_near(s_compound.x, 1.0);
    expect_near(s_compound.y, 2.0);

    // Unary negation
    SystemCoordinates s_neg = -s1;
    expect_near(s_neg.x, -10.5);
    expect_near(s_neg.y, 20.5);

    // Scalar multiplication and division
    SystemCoordinates s_mul = s2 * 2.0;
    expect_near(s_mul.x, 5.0);
    expect_near(s_mul.y, 11.0);

    SystemCoordinates s_mul_left = 2.0 * s2;
    expect_near(s_mul_left.x, 5.0);
    expect_near(s_mul_left.y, 11.0);

    SystemCoordinates s_div = s2 / 2.0;
    expect_near(s_div.x, 1.25);
    expect_near(s_div.y, 2.75);

    SystemCoordinates s_scale{4.0, 6.0};
    s_scale *= 0.5;
    expect_near(s_scale.x, 2.0);
    expect_near(s_scale.y, 3.0);
    s_scale /= 2.0;
    expect_near(s_scale.x, 1.0);
    expect_near(s_scale.y, 1.5);

    // Euclidean distance
    SystemCoordinates p_origin{0.0, 0.0};
    SystemCoordinates p_34{3.0, 4.0};
    expect_near(p_origin.distance_to(p_34), 5.0);
    expect_near(p_34.distance_to(p_origin), 5.0);

    // Bearing in radians
    expect_near(p_origin.bearing_to({10.0, 0.0}), 0.0);
    expect_near(p_origin.bearing_to({0.0, 10.0}), std::numbers::pi / 2.0);
    expect_near(p_origin.bearing_to({-10.0, 0.0}), std::numbers::pi);
    expect_near(p_origin.bearing_to({0.0, -10.0}), -std::numbers::pi / 2.0);

    // Comparisons
    test::expect_eq(s1, SystemCoordinates(10.5, -20.5));
    test::expect_ne(s1, s2);

    // Formatting
    test::expect_eq(std::format("{}", s1), "10.5,-20.5");
  }

  // --- UniverseCoordinates tests ---
  std::println(std::cout, "--- Testing UniverseCoordinates ---");
  {
    UniverseCoordinates u0{};
    test::expect_eq(u0.x, 0.0);
    test::expect_eq(u0.y, 0.0);

    UniverseCoordinates u1{1000.0, 2000.0};
    test::expect_eq(u1.x, 1000.0);
    test::expect_eq(u1.y, 2000.0);

    // Euclidean distance
    UniverseCoordinates u2{1600.0, 2800.0};
    expect_near(u1.distance_to(u2), 1000.0);
    expect_near(u2.distance_to(u1), 1000.0);

    // Bearing
    expect_near(u1.bearing_to({2000.0, 2000.0}), 0.0);
    expect_near(u1.bearing_to({1000.0, 3000.0}), std::numbers::pi / 2.0);

    // Comparisons
    test::expect_eq(u1, UniverseCoordinates(1000.0, 2000.0));
    test::expect_ne(u1, u2);

    // Formatting
    test::expect_eq(std::format("{}", u1), "1000,2000");
  }

  // --- Cross-frame arithmetic tests ---
  std::println(std::cout, "--- Testing Cross-Frame Operations ---");
  {
    UniverseCoordinates star_pos{5000.0, 10000.0};
    SystemCoordinates planet_offset{300.0, -400.0};

    // Adding system offset to universe coordinates yields universe coordinates
    UniverseCoordinates planet_abs = star_pos + planet_offset;
    expect_near(planet_abs.x, 5300.0);
    expect_near(planet_abs.y, 9600.0);

    // Commutative addition
    UniverseCoordinates planet_abs2 = planet_offset + star_pos;
    expect_near(planet_abs2.x, 5300.0);
    expect_near(planet_abs2.y, 9600.0);

    // Subtraction of system offset from universe position
    UniverseCoordinates star_sub = planet_abs - planet_offset;
    expect_near(star_sub.x, 5000.0);
    expect_near(star_sub.y, 10000.0);

    // Compound assignment
    UniverseCoordinates moving = star_pos;
    moving += planet_offset;
    expect_near(moving.x, 5300.0);
    expect_near(moving.y, 9600.0);
    moving -= planet_offset;
    expect_near(moving.x, 5000.0);
    expect_near(moving.y, 10000.0);

    // Difference of two UniverseCoordinates yields a SystemCoordinates
    // displacement vector
    SystemCoordinates displacement = planet_abs - star_pos;
    expect_near(displacement.x, 300.0);
    expect_near(displacement.y, -400.0);
    expect_near(displacement.distance_to({0.0, 0.0}), 500.0);

    // Static type assertions verifying affine frame discipline
    static_assert(std::is_same_v<decltype(star_pos + planet_offset),
                                 UniverseCoordinates>);
    static_assert(std::is_same_v<decltype(planet_offset + star_pos),
                                 UniverseCoordinates>);
    static_assert(std::is_same_v<decltype(star_pos - planet_offset),
                                 UniverseCoordinates>);
    static_assert(
        std::is_same_v<decltype(planet_abs - star_pos), SystemCoordinates>);
    static_assert(std::is_same_v<decltype(planet_offset + planet_offset),
                                 SystemCoordinates>);
    static_assert(std::is_same_v<decltype(planet_offset - planet_offset),
                                 SystemCoordinates>);
  }

  std::println(std::cout, "✓ All Coordinates & API Integration tests passed!");
  return 0;
}
