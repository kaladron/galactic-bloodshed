// SPDX-License-Identifier: Apache-2.0

/// \file strong_id_test.cc
/// \brief Unit tests for strong ID type wrapper, formatting, and hashing
/// operations.

import gblib;
import test;
import std;

int main() {
  // Test basic construction
  player_t player1{1};
  player_t player2{2};

  test::expect_eq(player1.value, 1);
  test::expect_eq(player2.value, 2);

  // Test comparison
  test::expect_ne(player1, player2);
  test::expect_lt(player1, player2);
  test::expect_gt(player2, player1);

  // Test type safety - these types are distinct
  shipnum_t ship{42};
  starnum_t star{5};

  test::expect_eq(ship.value, 42);
  test::expect_eq(star.value, 5);

  // Test increment/decrement
  player_t p{10};
  ++p;
  test::expect_eq(p.value, 11);
  p++;
  test::expect_eq(p.value, 12);
  --p;
  test::expect_eq(p.value, 11);

  // Test dereferencing
  test::expect_eq(*p, 11);

  // Test formatting
  std::string output =
      std::format("Player: {}, Ship: {}, Star: {}\n", player1, ship, star);
  test::expect_false(output.empty());
  std::println(std::cout, "{}", output);

  // Test hash support (for use in unordered containers)
  std::unordered_map<player_t, std::string> player_names;
  player_names[player1] = "Alice";
  player_names[player2] = "Bob";

  test::expect_eq(player_names[player1], "Alice");
  test::expect_eq(player_names[player2], "Bob");

  // Test to_underlying and underlying_type_t
  static_assert(std::is_same_v<underlying_type_t<player_t>, int>);
  test::expect_eq(to_underlying(player1), 1);
  test::expect_eq(to_underlying(42), 42);

  std::println(std::cout, "All strong_id tests passed!");
  return 0;
}
