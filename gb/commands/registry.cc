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
        auto reg = [&](const CommandDescriptor& desc) {
          std::string err;
          if (!validate_command_descriptor(desc, &err)) {
            throw std::logic_error(err);
          }
          map[desc.name] = &desc;
          for (const auto& alias : desc.aliases) {
            map[alias] = &desc;
          }
        };

        reg(shutdown_cmd);
        reg(purge_cmd);
        reg(bless_cmd);
        reg(capital_cmd);
        reg(governors_cmd);
        reg(help_cmd);
        reg(quit_cmd);
        reg(tax_cmd);
        reg(technology_cmd);
        reg(dock_cmd);
        reg(assault_cmd);
        reg(bombard_cmd);
        reg(capture_cmd);
        reg(defend_cmd);
        reg(detonate_cmd);
        reg(dissolve_cmd);
        reg(enslave_cmd);
        reg(insurgency_cmd);
        reg(jettison_cmd);
        reg(land_cmd);
        reg(launch_cmd);
        reg(load_cmd);
        reg(unload_cmd);
        reg(move_cmd);
        reg(deploy_cmd);
        reg(mount_cmd);
        reg(dismount_cmd);
        reg(map_cmd);
        reg(build_cmd);
        reg(fire_cmd);
        reg(fix_cmd);
        reg(cew_cmd);
        reg(cs_cmd);
        reg(center_cmd);
        reg(dump_cmd);
        reg(fuel_cmd);
        reg(orbit_cmd);
        reg(order_cmd);
        reg(repair_cmd);
        reg(route_cmd);
        reg(scrap_cmd);
        reg(sell_cmd);
        reg(survey_cmd);
        reg(client_survey_cmd);
        reg(tactical_cmd);
        reg(zoom_cmd);

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
