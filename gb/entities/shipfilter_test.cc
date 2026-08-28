// SPDX-License-Identifier: Apache-2.0

/// \file shipfilter_test.cc
/// \brief Test ship filtering helper functions

import dallib;
import gb.entities;
import test;
import std;

int main() {
  // Test parse_ship_selection
  {
    auto a = GB::parse_ship_selection("123");
    test::expect_true(a.has_value());
    test::expect_eq(a.value(), 123);

    auto b = GB::parse_ship_selection("#456");
    test::expect_true(b.has_value());
    test::expect_eq(b.value(), 456);

    auto c = GB::parse_ship_selection("##789");
    test::expect_true(c.has_value());
    test::expect_eq(c.value(), 789);

    auto d = GB::parse_ship_selection("f");
    test::expect_false(d.has_value());

    auto e = GB::parse_ship_selection("*");
    test::expect_false(e.has_value());

    auto f = GB::parse_ship_selection("");
    test::expect_false(f.has_value());

    auto g = GB::parse_ship_selection("frd");
    test::expect_false(g.has_value());
  }

  // Test is_ship_number_filter
  {
    test::expect_true(GB::is_ship_number_filter("#123"));
    test::expect_true(GB::is_ship_number_filter("#456"));
    test::expect_false(GB::is_ship_number_filter(
        "123"));  // Without '#', it's a ship type filter
    test::expect_false(GB::is_ship_number_filter("f"));
    test::expect_false(GB::is_ship_number_filter("*"));
    test::expect_false(GB::is_ship_number_filter(""));
    test::expect_false(GB::is_ship_number_filter("frd"));
  }

  // Test ship_matches_filter with actual ships
  {
    Database db(":memory:");
    initialize_schema(db);
    EntityManager em(db);

    // Create test ships
    Ship pod;
    pod.number() = 1;
    pod.type() = ShipType::STYPE_POD;  // 'p' at index 0
    pod.owner() = 1;
    pod.alive() = true;

    Ship destroyer;
    destroyer.number() = 2;
    destroyer.type() = ShipType::STYPE_DESTROYER;  // 'd' at index 7
    destroyer.owner() = 1;
    destroyer.alive() = true;

    Ship fighter;
    fighter.number() = 3;
    fighter.type() = ShipType::STYPE_FIGHTER;  // 'f' at index 8
    fighter.owner() = 1;
    fighter.alive() = true;

    // Test single ship type filter
    std::println(std::cout, "Testing pod: type={}, Shipltrs[type]='{}'",
                 static_cast<int>(pod.type()), Shipltrs[pod.type()]);
    std::println(std::cout, "Filter 'p' matches pod: {}",
                 GB::ship_matches_filter("p", pod));
    test::expect_true(GB::ship_matches_filter("p", pod));
    test::expect_false(GB::ship_matches_filter("p", destroyer));
    test::expect_false(GB::ship_matches_filter("p", fighter));

    // Test multi-ship type filter (p=pod, d=destroyer)
    test::expect_true(GB::ship_matches_filter("pd", pod));
    test::expect_true(GB::ship_matches_filter("pd", destroyer));
    test::expect_false(GB::ship_matches_filter("pd", fighter));

    test::expect_true(GB::ship_matches_filter("fdp", pod));
    test::expect_true(GB::ship_matches_filter("fdp", destroyer));
    test::expect_true(GB::ship_matches_filter("fdp", fighter));

    // Test wildcard filter
    test::expect_true(GB::ship_matches_filter("*", pod));
    test::expect_true(GB::ship_matches_filter("*", destroyer));
    test::expect_true(GB::ship_matches_filter("*", fighter));

    // Test ship number filter - now checks if specific ship number matches
    test::expect_true(GB::ship_matches_filter("#1", pod));  // pod is ship #1
    test::expect_false(
        GB::ship_matches_filter("#1", destroyer));  // destroyer is ship #2
    test::expect_true(
        GB::ship_matches_filter("#2", destroyer));  // destroyer is ship #2
    test::expect_false(
        GB::ship_matches_filter("#123", pod));  // no ship #123 in this set

    // Numeric strings WITHOUT '#' are treated as ship type filters
    // They look for ships with type letters matching the digits
    test::expect_false(GB::ship_matches_filter(
        "123", pod));  // pod is 'p', not '1', '2', or '3'

    // Test empty filter
    test::expect_false(GB::ship_matches_filter("", pod));
    test::expect_false(GB::ship_matches_filter("", destroyer));
  }

  std::println(std::cout, "All shipfilter tests passed!");
  return 0;
}
