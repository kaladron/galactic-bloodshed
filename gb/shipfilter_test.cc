// SPDX-License-Identifier: Apache-2.0

/// \file shipfilter_test.cc
/// \brief Test ship filtering helper functions

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Test parse_ship_selection
  {
    auto a = GB::parse_ship_selection("123");
    assert(a.has_value() && a.value() == 123);

    auto b = GB::parse_ship_selection("#456");
    assert(b.has_value() && b.value() == 456);

    auto c = GB::parse_ship_selection("##789");
    assert(c.has_value() && c.value() == 789);

    auto d = GB::parse_ship_selection("f");
    assert(!d.has_value());

    auto e = GB::parse_ship_selection("*");
    assert(!e.has_value());

    auto f = GB::parse_ship_selection("");
    assert(!f.has_value());

    auto g = GB::parse_ship_selection("frd");
    assert(!g.has_value());
  }

  // Test is_ship_number_filter
  {
    assert(GB::is_ship_number_filter("#123") == true);
    assert(GB::is_ship_number_filter("#456") == true);
    assert(GB::is_ship_number_filter("123") ==
           false);  // Without '#', it's a ship type filter
    assert(GB::is_ship_number_filter("f") == false);
    assert(GB::is_ship_number_filter("*") == false);
    assert(GB::is_ship_number_filter("") == false);
    assert(GB::is_ship_number_filter("frd") == false);
  }

  // Test ship_matches_filter with actual ships
  {
    Database db(":memory:");
    initialize_schema(db);
    EntityManager em(db);

    // Create test ships
    Ship pod;
    pod.number = 1;
    pod.type = ShipType::STYPE_POD;  // 'p' at index 0
    pod.owner = 1;
    pod.alive = true;

    Ship destroyer;
    destroyer.number = 2;
    destroyer.type = ShipType::STYPE_DESTROYER;  // 'd' at index 7
    destroyer.owner = 1;
    destroyer.alive = true;

    Ship fighter;
    fighter.number = 3;
    fighter.type = ShipType::STYPE_FIGHTER;  // 'f' at index 8
    fighter.owner = 1;
    fighter.alive = true;

    // Test single ship type filter
    std::println("Testing pod: type={}, Shipltrs[type]='{}'",
                 static_cast<int>(pod.type), Shipltrs[pod.type]);
    std::println("Filter 'p' matches pod: {}",
                 GB::ship_matches_filter("p", pod));
    assert(GB::ship_matches_filter("p", pod) == true);
    assert(GB::ship_matches_filter("p", destroyer) == false);
    assert(GB::ship_matches_filter("p", fighter) == false);

    // Test multi-ship type filter (p=pod, d=destroyer)
    assert(GB::ship_matches_filter("pd", pod) == true);
    assert(GB::ship_matches_filter("pd", destroyer) == true);
    assert(GB::ship_matches_filter("pd", fighter) == false);

    assert(GB::ship_matches_filter("fdp", pod) == true);
    assert(GB::ship_matches_filter("fdp", destroyer) == true);
    assert(GB::ship_matches_filter("fdp", fighter) == true);

    // Test wildcard filter
    assert(GB::ship_matches_filter("*", pod) == true);
    assert(GB::ship_matches_filter("*", destroyer) == true);
    assert(GB::ship_matches_filter("*", fighter) == true);

    // Test ship number filter - now checks if specific ship number matches
    assert(GB::ship_matches_filter("#1", pod) == true);  // pod is ship #1
    assert(GB::ship_matches_filter("#1", destroyer) ==
           false);  // destroyer is ship #2
    assert(GB::ship_matches_filter("#2", destroyer) ==
           true);  // destroyer is ship #2
    assert(GB::ship_matches_filter("#123", pod) ==
           false);  // no ship #123 in this set

    // Numeric strings WITHOUT '#' are treated as ship type filters
    // They look for ships with type letters matching the digits
    assert(GB::ship_matches_filter("123", pod) ==
           false);  // pod is 'p', not '1', '2', or '3'

    // Test empty filter
    assert(GB::ship_matches_filter("", pod) == false);
    assert(GB::ship_matches_filter("", destroyer) == false);
  }

  std::println("All shipfilter tests passed!");
  return 0;
}
