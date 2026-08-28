// SPDX-License-Identifier: Apache-2.0

export module commands:spec;

import gb.entities;
import gb.services;
import std;

namespace GB::commands {

/// Role and privilege requirements for a command
export struct RoleRequirements {
  bool god_only = false;      ///< Must have active deity/god privileges
  bool no_guests = false;     ///< Guest races are prohibited
  bool leader_only = false;   ///< Only Governor 0 (leader)
  bool star_control = false;  ///< Must control the current star system
};

/// Allowed scope levels for command execution
export struct AllowedScopes {
  bool univ = false;
  bool star = false;
  bool planet = false;
  bool ship = false;

  [[nodiscard]] constexpr bool allows(ScopeLevel level) const {
    switch (level) {
      case ScopeLevel::LEVEL_UNIV:
        return univ;
      case ScopeLevel::LEVEL_STAR:
        return star;
      case ScopeLevel::LEVEL_PLAN:
        return planet;
      case ScopeLevel::LEVEL_SHIP:
        return ship;
    }
    return false;
  }

  // Pre-configured standard presets
  static constexpr AllowedScopes any() {
    return {true, true, true, true};
  }
  static constexpr AllowedScopes planet_or_ship() {
    return {.planet = true, .ship = true};
  }
  static constexpr AllowedScopes planet_only() {
    return {.planet = true};
  }
  static constexpr AllowedScopes ship_only() {
    return {.ship = true};
  }
  static constexpr AllowedScopes star_only() {
    return {.star = true};
  }
  static constexpr AllowedScopes star_or_univ() {
    return {.univ = true, .star = true};
  }
  static constexpr AllowedScopes non_universe() {
    return {.star = true, .planet = true, .ship = true};
  }
};

/// AP cost configuration
export enum class APModel {
  Free,       ///< 0 AP cost
  FixedStar,  ///< Fixed cost deducted from current star system
  FixedUniv,  ///< Fixed cost deducted from universe AP pool
  Dynamic     ///< Dynamic cost (deducted per-action via GameObj::deduct_ap)
};

export struct APCost {
  APModel model = APModel::Free;
  ap_t amount = 0;

  static constexpr APCost free() {
    return {APModel::Free, 0};
  }
  static constexpr APCost fixed_star(ap_t cost) {
    return {APModel::FixedStar, cost};
  }
  static constexpr APCost fixed_univ(ap_t cost) {
    return {APModel::FixedUniv, cost};
  }
  static constexpr APCost dynamic() {
    return {APModel::Dynamic, 0};
  }
};

/// Unified command function signature across all commands
export using CommandFn = bool (*)(const command_t& argv, GameObj& g);

export struct CommandDescriptor {
  std::string_view name;
  std::span<const std::string_view> aliases;
  RoleRequirements roles = {};
  AllowedScopes scopes = AllowedScopes::any();
  APCost ap = APCost::free();
  std::size_t min_args = 1;
  std::string_view syntax = "";
  std::string_view description = "";
  CommandFn handler = nullptr;
};

/// Centralized command dispatch pipeline executing validation and AP management
export bool dispatch_command(GameObj& g, const CommandDescriptor& desc,
                             const command_t& argv);

/// Validates command descriptor invariants (non-empty name, non-null handler,
/// valid scopes, syntax present when args required, and AP amount consistency).
export bool validate_command_descriptor(const CommandDescriptor& desc,
                                        std::string* error = nullptr);

/// Retrieve the global command descriptor registry.
export const std::unordered_map<std::string_view, const CommandDescriptor*>&
get_command_registry();

/// Look up a command descriptor by name or alias.
export const CommandDescriptor* find_command_descriptor(std::string_view name);

}  // namespace GB::commands
