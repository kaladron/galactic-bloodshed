// SPDX-License-Identifier: Apache-2.0

module;

import std;

module commands;

namespace GB::commands {

bool validate_command_descriptor(const CommandDescriptor& desc,
                                 std::string* error) {
  if (desc.name.empty()) {
    if (error) *error = "Command descriptor name must not be empty.";
    return false;
  }
  if (desc.handler == nullptr) {
    if (error) {
      *error = std::format("Command '{}' has null handler.", desc.name);
    }
    return false;
  }
  bool has_scope = desc.scopes.univ || desc.scopes.star || desc.scopes.planet ||
                   desc.scopes.ship;
  if (!has_scope) {
    if (error) {
      *error = std::format("Command '{}' must allow at least one scope level.",
                           desc.name);
    }
    return false;
  }
  if (desc.min_args > 1 && desc.syntax.empty()) {
    if (error) {
      *error = std::format(
          "Command '{}' requires arguments (min_args = {}) but provides no "
          "syntax string.",
          desc.name, desc.min_args);
    }
    return false;
  }
  if (desc.ap.model == APModel::FixedStar ||
      desc.ap.model == APModel::FixedUniv) {
    if (desc.ap.amount == 0) {
      if (error) {
        *error = std::format(
            "Command '{}' uses fixed AP model but declares 0 AP cost.",
            desc.name);
      }
      return false;
    }
  } else {
    if (desc.ap.amount != 0) {
      if (error) {
        *error = std::format(
            "Command '{}' uses Free or Dynamic AP model but declares non-zero "
            "amount ({}).",
            desc.name, desc.ap.amount);
      }
      return false;
    }
  }
  return true;
}

namespace {

const std::unordered_map<std::string_view, const CommandDescriptor*>&
build_registry() {
  static const std::unordered_map<std::string_view, const CommandDescriptor*>
      registry = [] {
        std::unordered_map<std::string_view, const CommandDescriptor*> map;
        // As player commands are migrated across subsequent commits, their
        // descriptors will be registered in this table.
        return map;
      }();
  return registry;
}

}  // namespace

const std::unordered_map<std::string_view, const CommandDescriptor*>&
get_command_registry() {
  return build_registry();
}

const CommandDescriptor* find_command_descriptor(std::string_view name) {
  const auto& reg = get_command_registry();
  auto it = reg.find(name);
  if (it != reg.end()) {
    return it->second;
  }
  return nullptr;
}

}  // namespace GB::commands
