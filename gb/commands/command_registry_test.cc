// SPDX-License-Identifier: Apache-2.0

/// \file command_registry_test.cc
/// \brief Unit tests for command registry invariants and descriptor validation

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

bool dummy_handler(const command_t&, GameObj&) {
  return true;
}

void test_live_registry_integrity() {
  const auto& registry = GB::commands::get_command_registry();
  std::set<std::string_view> seen_keys;

  for (const auto& [key, desc_ptr] : registry) {
    test::expect_ne(desc_ptr, nullptr);
    const auto& desc = *desc_ptr;

    // 1. Validate descriptor invariants
    std::string error;
    bool valid = GB::commands::validate_command_descriptor(desc, &error);
    test::expect_true(valid, error);

    // 2. Key must match either primary name or an alias
    bool matches_name = (key == desc.name);
    bool matches_alias = false;
    for (const auto& alias : desc.aliases) {
      if (key == alias) {
        matches_alias = true;
        break;
      }
    }
    test::expect_true(matches_name || matches_alias,
                      "Key must match name or alias");

    // 3. No duplicate keys
    test::expect_false(seen_keys.contains(key),
                       "Duplicate key in command registry");
    seen_keys.insert(key);

    // 4. Lookup by key returns the exact same descriptor pointer
    test::expect_eq(GB::commands::find_command_descriptor(key), desc_ptr);
  }
}

void test_find_command_descriptor_not_found() {
  test::expect_eq(GB::commands::find_command_descriptor("nonexistent_cmd_xyz"),
                  nullptr);
  test::expect_eq(GB::commands::find_command_descriptor(""), nullptr);
}

void test_descriptor_validation_invariants() {
  // 1. Valid descriptor
  GB::commands::CommandDescriptor valid_desc{
      .name = "test_cmd",
      .roles = {.no_guests = true},
      .scopes = GB::commands::AllowedScopes::planet_only(),
      .ap = GB::commands::APCost::fixed_star(1),
      .min_args = 2,
      .syntax = "test_cmd <arg>",
      .description = "Test description",
      .handler = &dummy_handler,
  };
  std::string error;
  test::expect_true(
      GB::commands::validate_command_descriptor(valid_desc, &error));
  test::expect_true(error.empty());

  // 2. Empty name
  auto empty_name_desc = valid_desc;
  empty_name_desc.name = "";
  test::expect_false(
      GB::commands::validate_command_descriptor(empty_name_desc, &error));
  test::expect_contains(error, "must not be empty");

  // 3. Null handler
  auto null_handler_desc = valid_desc;
  null_handler_desc.handler = nullptr;
  test::expect_false(
      GB::commands::validate_command_descriptor(null_handler_desc, &error));
  test::expect_contains(error, "has null handler");

  // 4. Zero scopes
  auto zero_scopes_desc = valid_desc;
  zero_scopes_desc.scopes = {};
  test::expect_false(
      GB::commands::validate_command_descriptor(zero_scopes_desc, &error));
  test::expect_contains(error, "must allow at least one scope level");

  // 5. Missing syntax when min_args > 1
  auto missing_syntax_desc = valid_desc;
  missing_syntax_desc.min_args = 2;
  missing_syntax_desc.syntax = "";
  test::expect_false(
      GB::commands::validate_command_descriptor(missing_syntax_desc, &error));
  test::expect_contains(error, "requires arguments");
  test::expect_contains(error, "syntax string");

  // Syntax is optional when min_args <= 1
  auto single_arg_desc = valid_desc;
  single_arg_desc.min_args = 1;
  single_arg_desc.syntax = "";
  test::expect_true(
      GB::commands::validate_command_descriptor(single_arg_desc, &error));

  // 6. Fixed AP with 0 amount
  auto zero_fixed_star_desc = valid_desc;
  zero_fixed_star_desc.ap = {GB::commands::APModel::FixedStar, 0};
  test::expect_false(
      GB::commands::validate_command_descriptor(zero_fixed_star_desc, &error));
  test::expect_contains(error, "fixed AP model but declares 0 AP cost");

  auto zero_fixed_univ_desc = valid_desc;
  zero_fixed_univ_desc.ap = {GB::commands::APModel::FixedUniv, 0};
  test::expect_false(
      GB::commands::validate_command_descriptor(zero_fixed_univ_desc, &error));
  test::expect_contains(error, "fixed AP model but declares 0 AP cost");

  // 7. Free / Dynamic AP with non-zero amount
  auto non_zero_free_desc = valid_desc;
  non_zero_free_desc.ap = {GB::commands::APModel::Free, 5};
  test::expect_false(
      GB::commands::validate_command_descriptor(non_zero_free_desc, &error));
  test::expect_contains(
      error, "Free or Dynamic AP model but declares non-zero amount");

  auto non_zero_dyn_desc = valid_desc;
  non_zero_dyn_desc.ap = {GB::commands::APModel::Dynamic, 5};
  test::expect_false(
      GB::commands::validate_command_descriptor(non_zero_dyn_desc, &error));
  test::expect_contains(
      error, "Free or Dynamic AP model but declares non-zero amount");
}

}  // namespace

int main() {
  test_live_registry_integrity();
  test_find_command_descriptor_not_found();
  test_descriptor_validation_invariants();

  std::println(std::cout, "✓ command_registry_test passed!");
  return 0;
}
