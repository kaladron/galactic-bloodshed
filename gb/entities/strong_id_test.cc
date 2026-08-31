// SPDX-License-Identifier: Apache-2.0

/// \file strong_id_test.cc
/// \brief Unit tests for strong ID type wrapper, formatting, and hashing
/// operations.

import gb.entities;
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

  // ---------------------------------------------------------------------------
  // Bounded<Tag, T, Min, Max> Tests
  // ---------------------------------------------------------------------------
  {
    // Construction clamping
    bounded_damage_t d_normal{50};
    bounded_damage_t d_underflow{0};
    bounded_damage_t d_negative{static_cast<std::uint32_t>(
        -10)};  // large unsigned wraps down to max or up from min
    bounded_damage_t d_overflow{150};

    test::expect_eq(d_normal.value, 50u);
    test::expect_eq(d_underflow.value, 0u);
    test::expect_eq(d_negative.value, 100u);
    test::expect_eq(d_overflow.value, 100u);
    test::expect_eq(bounded_damage_t::min(), 0u);
    test::expect_eq(bounded_damage_t::max(), 100u);

    // Signed bounded test
    morale_t m_normal{25};
    morale_t m_negative{-50};
    morale_t m_over{120};
    test::expect_eq(m_normal.value, 25);
    test::expect_eq(m_negative.value, 0);
    test::expect_eq(m_over.value, 100);

    // Mutating arithmetic clamping
    bounded_damage_t d{90};
    d += 20;
    test::expect_eq(d.value, 100u);  // Clamped at 100
    d -= 150;
    test::expect_eq(d.value, 0u);  // Clamped at 0
    d += 10;
    d *= 5;
    test::expect_eq(d.value, 50u);
    d /= 2;
    test::expect_eq(d.value, 25u);

    // Increment / Decrement
    bounded_speed_t sp{8};
    ++sp;
    test::expect_eq(sp.value, 9u);
    ++sp;  // Saturated at Max (9)
    test::expect_eq(sp.value, 9u);
    --sp;
    test::expect_eq(sp.value, 8u);

    // Binary operators
    bounded_damage_t d2 = d + 30;
    test::expect_eq(d2.value, 55u);
    bounded_damage_t d3 = d - 100;
    test::expect_eq(d3.value, 0u);

    // Formatting & streaming
    std::string d_str = std::format("Damage: {}", d2);
    test::expect_eq(d_str, "Damage: 55");

    // Hash support
    std::unordered_map<efficiency_t, std::string> eff_map;
    efficiency_t eff{80};
    eff_map[eff] = "High";
    test::expect_eq(eff_map[efficiency_t{80}], "High");

    // Numeric limits
    test::expect_eq(std::numeric_limits<bounded_damage_t>::min().value, 0u);
    test::expect_eq(std::numeric_limits<bounded_damage_t>::max().value, 100u);
  }

  // ---------------------------------------------------------------------------
  // Modular<Tag, T, Mod> Tests
  // ---------------------------------------------------------------------------
  {
    // Construction wrapping (0..359)
    modular_bearing_t b0{0};
    modular_bearing_t b180{180};
    modular_bearing_t b360{360};
    modular_bearing_t b725{725};

    test::expect_eq(b0.value, 0u);
    test::expect_eq(b180.value, 180u);
    test::expect_eq(b360.value, 0u);  // 360 % 360 = 0
    test::expect_eq(b725.value, 5u);  // 725 % 360 = 5
    test::expect_eq(modular_bearing_t::modulus(), 360u);

    // Mutating arithmetic wrapping
    modular_bearing_t b{350};
    b += 20;
    test::expect_eq(b.value, 10u);  // (350 + 20) % 360 = 10
    b -= 30;
    test::expect_eq(b.value, 340u);  // (10 - 30 + 360) % 360 = 340

    // Increment / Decrement
    modular_bearing_t b_edge{359};
    ++b_edge;
    test::expect_eq(b_edge.value, 0u);  // Wrapped from 359 to 0
    --b_edge;
    test::expect_eq(b_edge.value, 359u);  // Wrapped from 0 to 359

    // Formatting & streaming
    std::string b_str = std::format("Bearing: {} deg", b);
    test::expect_eq(b_str, "Bearing: 340 deg");

    // Hash support
    std::unordered_map<modular_bearing_t, std::string> heading_map;
    heading_map[modular_bearing_t{90}] = "East";
    test::expect_eq(heading_map[modular_bearing_t{90}], "East");
  }

  std::println(std::cout, "All strong_id, bounded, and modular tests passed!");
  return 0;
}
