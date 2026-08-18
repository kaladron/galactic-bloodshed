// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

bool dummy_handler(const command_t&, GameObj&) {
  return true;
}

void test_live_registry_integrity() {
  const auto& registry = GB::commands::get_command_registry();
  std::set<std::string_view> seen_keys;

  for (const auto& [key, desc_ptr] : registry) {
    assert(desc_ptr != nullptr);
    const auto& desc = *desc_ptr;

    // 1. Validate descriptor invariants
    std::string error;
    bool valid = GB::commands::validate_command_descriptor(desc, &error);
    assert(valid && error.c_str());

    // 2. Key must match either primary name or an alias
    bool matches_name = (key == desc.name);
    bool matches_alias = false;
    for (const auto& alias : desc.aliases) {
      if (key == alias) {
        matches_alias = true;
        break;
      }
    }
    assert((matches_name || matches_alias) && "Key must match name or alias");

    // 3. No duplicate keys
    assert(!seen_keys.contains(key) && "Duplicate key in command registry");
    seen_keys.insert(key);

    // 4. Lookup by key returns the exact same descriptor pointer
    assert(GB::commands::find_command_descriptor(key) == desc_ptr);
  }
}

void test_find_command_descriptor_not_found() {
  assert(GB::commands::find_command_descriptor("nonexistent_cmd_xyz") ==
         nullptr);
  assert(GB::commands::find_command_descriptor("") == nullptr);
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
  assert(GB::commands::validate_command_descriptor(valid_desc, &error));
  assert(error.empty());

  // 2. Empty name
  auto empty_name_desc = valid_desc;
  empty_name_desc.name = "";
  assert(!GB::commands::validate_command_descriptor(empty_name_desc, &error));
  assert(error.contains("must not be empty"));

  // 3. Null handler
  auto null_handler_desc = valid_desc;
  null_handler_desc.handler = nullptr;
  assert(!GB::commands::validate_command_descriptor(null_handler_desc, &error));
  assert(error.contains("has null handler"));

  // 4. Zero scopes
  auto zero_scopes_desc = valid_desc;
  zero_scopes_desc.scopes = {};
  assert(!GB::commands::validate_command_descriptor(zero_scopes_desc, &error));
  assert(error.contains("must allow at least one scope level"));

  // 5. Missing syntax when min_args > 1
  auto missing_syntax_desc = valid_desc;
  missing_syntax_desc.min_args = 2;
  missing_syntax_desc.syntax = "";
  assert(
      !GB::commands::validate_command_descriptor(missing_syntax_desc, &error));
  assert(error.contains("requires arguments") &&
         error.contains("syntax string"));

  // Syntax is optional when min_args <= 1
  auto single_arg_desc = valid_desc;
  single_arg_desc.min_args = 1;
  single_arg_desc.syntax = "";
  assert(GB::commands::validate_command_descriptor(single_arg_desc, &error));

  // 6. Fixed AP with 0 amount
  auto zero_fixed_star_desc = valid_desc;
  zero_fixed_star_desc.ap = {GB::commands::APModel::FixedStar, 0};
  assert(
      !GB::commands::validate_command_descriptor(zero_fixed_star_desc, &error));
  assert(error.contains("fixed AP model but declares 0 AP cost"));

  auto zero_fixed_univ_desc = valid_desc;
  zero_fixed_univ_desc.ap = {GB::commands::APModel::FixedUniv, 0};
  assert(
      !GB::commands::validate_command_descriptor(zero_fixed_univ_desc, &error));
  assert(error.contains("fixed AP model but declares 0 AP cost"));

  // 7. Free / Dynamic AP with non-zero amount
  auto non_zero_free_desc = valid_desc;
  non_zero_free_desc.ap = {GB::commands::APModel::Free, 5};
  assert(
      !GB::commands::validate_command_descriptor(non_zero_free_desc, &error));
  assert(
      error.contains("Free or Dynamic AP model but declares non-zero amount"));

  auto non_zero_dyn_desc = valid_desc;
  non_zero_dyn_desc.ap = {GB::commands::APModel::Dynamic, 5};
  assert(!GB::commands::validate_command_descriptor(non_zero_dyn_desc, &error));
  assert(
      error.contains("Free or Dynamic AP model but declares non-zero amount"));
}

}  // namespace

int main() {
  test_live_registry_integrity();
  test_find_command_descriptor_not_found();
  test_descriptor_validation_invariants();

  std::println(std::cout, "✓ command_registry_test passed!");
  return 0;
}
