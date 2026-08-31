// SPDX-License-Identifier: Apache-2.0

/// \file strong_id_json_test.cc
/// \brief Unit tests for Glaze serialization and deserialization of strong ID
/// types.

import strong_id;
import glaze.core;
import glaze.json;
import test;
import std;

// Test that strong ID types serialize/deserialize correctly with glaze
// This validates the glz::meta<ID<Tag,T>> specialization BEFORE
// we change governor_t to use it.

// Create a test ID type (don't use governor_t yet - it's still std::uint32_t)
using test_id_t = ID<"test", int>;

// Glaze serialization support for strong ID, Bounded, and Modular types
namespace glz {
template <FixedString Tag, typename T>
struct from<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(ID<Tag, T>& id, is_context auto&& ctx, auto&& it, auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    id = ID<Tag, T>{val};
  }
};

template <FixedString Tag, typename T>
struct to<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(const ID<Tag, T>& id, is_context auto&& ctx, auto&& b,
                 auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(id.value, ctx, b, ix);
  }
};

template <FixedString Tag, typename T, T Min, T Max>
struct from<JSON, Bounded<Tag, T, Min, Max>> {
  template <auto Opts>
  static void op(Bounded<Tag, T, Min, Max>& b, is_context auto&& ctx, auto&& it,
                 auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    b = Bounded<Tag, T, Min, Max>{val};
  }
};

template <FixedString Tag, typename T, T Min, T Max>
struct to<JSON, Bounded<Tag, T, Min, Max>> {
  template <auto Opts>
  static void op(const Bounded<Tag, T, Min, Max>& b, is_context auto&& ctx,
                 auto&& buf, auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(b.value, ctx, buf, ix);
  }
};

template <FixedString Tag, typename T, T Mod>
struct from<JSON, Modular<Tag, T, Mod>> {
  template <auto Opts>
  static void op(Modular<Tag, T, Mod>& m, is_context auto&& ctx, auto&& it,
                 auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    m = Modular<Tag, T, Mod>{val};
  }
};

template <FixedString Tag, typename T, T Mod>
struct to<JSON, Modular<Tag, T, Mod>> {
  template <auto Opts>
  static void op(const Modular<Tag, T, Mod>& m, is_context auto&& ctx,
                 auto&& buf, auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(m.value, ctx, buf, ix);
  }
};
}  // namespace glz

using test_bounded_t = Bounded<"test_damage", std::uint32_t, 0, 100>;
using test_modular_t = Modular<"test_bearing", std::uint32_t, 360>;

// Simple struct containing our test types
struct TestStruct {
  test_id_t id;
  std::string name;
  test_bounded_t damage{0};
  test_modular_t bearing{0};
};

// Glaze metadata for TestStruct
namespace glz {
template <>
struct meta<TestStruct> {
  using T = TestStruct;
  static constexpr auto value = object("id", &T::id, "name", &T::name, "damage",
                                       &T::damage, "bearing", &T::bearing);
};
}  // namespace glz

int main() {
  // Write ID to JSON
  {
    test_id_t id{42};
    auto result = glz::write_json(id);
    test::expect_true(result.has_value());
    test::expect_eq(result.value(), "42");
    std::println(std::cout, "✓ Strong ID serializes as plain integer: {}",
                 result.value());
  }

  // Read ID from JSON
  {
    test_id_t id{0};
    auto ec = glz::read_json(id, "123");
    test::expect_false(bool(ec));
    test::expect_eq(id.value, 123);
    std::println(std::cout, "✓ Strong ID deserializes from plain integer: {}",
                 id.value);
  }

  // Bounded and Modular direct serialization
  {
    test_bounded_t dmg{75};
    auto res_dmg = glz::write_json(dmg);
    test::expect_true(res_dmg.has_value());
    test::expect_eq(res_dmg.value(), "75");

    test_modular_t brg{180};
    auto res_brg = glz::write_json(brg);
    test::expect_true(res_brg.has_value());
    test::expect_eq(res_brg.value(), "180");

    test_bounded_t parsed_dmg{0};
    auto ec1 =
        glz::read_json(parsed_dmg, "250");  // Exceeds max 100 -> clamped to 100
    test::expect_false(bool(ec1));
    test::expect_eq(parsed_dmg.value, 100u);

    test_modular_t parsed_brg{0};
    auto ec2 = glz::read_json(parsed_brg, "720");  // 720 % 360 -> 0
    test::expect_false(bool(ec2));
    test::expect_eq(parsed_brg.value, 0u);
  }

  // Round-trip struct containing ID, Bounded, and Modular
  {
    TestStruct original{
        .id = test_id_t{99},
        .name = "test",
        .damage = test_bounded_t{45},
        .bearing = test_modular_t{270},
    };

    auto json_result = glz::write_json(original);
    test::expect_true(json_result.has_value());
    std::println(std::cout, "✓ Struct JSON: {}", json_result.value());

    TestStruct parsed{};
    auto ec = glz::read_json(parsed, json_result.value());
    test::expect_false(bool(ec));
    test::expect_eq(parsed.id.value, 99);
    test::expect_eq(parsed.name, "test");
    test::expect_eq(parsed.damage.value, 45u);
    test::expect_eq(parsed.bearing.value, 270u);
    std::println(std::cout, "✓ Struct round-trip successful");
  }

  // Verify backward compatibility format
  // Existing JSON stores values as plain integers
  {
    TestStruct parsed{};
    auto ec = glz::read_json(
        parsed, R"({"id": 5, "name": "compat", "damage": 80, "bearing": 90})");
    test::expect_false(bool(ec));
    test::expect_eq(parsed.id.value, 5);
    test::expect_eq(parsed.damage.value, 80u);
    test::expect_eq(parsed.bearing.value, 90u);
    std::println(std::cout, "✓ Backward compatible with existing JSON format");
  }

  std::println(std::cout, "\n✅ All strong ID, Bounded, and Modular JSON "
                          "serialization tests passed!");
  return 0;
}
