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

// Glaze serialization support for strong ID types
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
}  // namespace glz

// Simple struct containing our test ID
struct TestStruct {
  test_id_t id;
  std::string name;
};

// Glaze metadata for TestStruct
namespace glz {
template <>
struct meta<TestStruct> {
  using T = TestStruct;
  static constexpr auto value = object("id", &T::id, "name", &T::name);
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

  // Round-trip struct containing ID
  {
    TestStruct original{.id = test_id_t{99}, .name = "test"};

    auto json_result = glz::write_json(original);
    test::expect_true(json_result.has_value());
    std::println(std::cout, "✓ Struct JSON: {}", json_result.value());

    TestStruct parsed{};
    auto ec = glz::read_json(parsed, json_result.value());
    test::expect_false(bool(ec));
    test::expect_eq(parsed.id.value, 99);
    test::expect_eq(parsed.name, "test");
    std::println(std::cout, "✓ Struct round-trip successful");
  }

  // Verify backward compatibility format
  // Existing JSON stores governors as plain integers like: "governor": 2
  {
    TestStruct parsed{};
    auto ec = glz::read_json(parsed, R"({"id": 5, "name": "compat"})");
    test::expect_false(bool(ec));
    test::expect_eq(parsed.id.value, 5);
    std::println(std::cout, "✓ Backward compatible with existing JSON format");
  }

  std::println(std::cout,
               "\n✅ All strong ID JSON serialization tests passed!");
  return 0;
}
