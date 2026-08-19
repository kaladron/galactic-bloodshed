// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool dispatch_command(GameObj& g, const CommandDescriptor& desc,
                      const command_t& argv) {
  // 1. Role & Permission Verification
  if (desc.roles.god_only && !g.god()) {
    g.out << "Only deity can use this command.\n";
    return false;
  }
  if (desc.roles.no_guests && g.race && g.race->Guest) {
    g.out << "Guest races cannot use this command.\n";
    return false;
  }
  if (desc.roles.leader_only && g.governor() != 0) {
    g.out << "Only the leader (Governor 0) may use this command.\n";
    return false;
  }
  if (desc.roles.star_control) {
    try {
      const auto* star = g.entity_manager.peek_star(g.snum());
      if (!star || !star->control(g.player(), g.governor())) {
        g.out << "You are not authorized to do that in this system.\n";
        return false;
      }
    } catch (const EntityNotFoundError&) {
      g.out << "You are not authorized to do that in this system.\n";
      return false;
    }
  }

  // 2. Scope Verification
  if (!desc.scopes.allows(g.level())) {
    g.out << "Invalid scope for this command.\n";
    return false;
  }

  // 3. Argument Count Check
  if (argv.size() < desc.min_args) {
    g.out << std::format("Syntax: {}\n", desc.syntax);
    return false;
  }

  // 4. Fixed-Cost AP Pre-check
  if (desc.ap.model == APModel::FixedStar) {
    try {
      const auto* star = g.entity_manager.peek_star(g.snum());
      if (!star || star->AP(g.player()) < desc.ap.amount) {
        g.out << std::format("You don't have {} action points there.\n",
                             desc.ap.amount);
        return false;
      }
    } catch (const EntityNotFoundError&) {
      g.out << std::format("You don't have {} action points there.\n",
                           desc.ap.amount);
      return false;
    }
  } else if (desc.ap.model == APModel::FixedUniv) {
    const auto* univ = g.entity_manager.peek_universe();
    if (!univ || univ->AP[g.player().value - 1] < desc.ap.amount) {
      g.out << std::format("You need {} universe action points.\n",
                           desc.ap.amount);
      return false;
    }
  }

  // 5. Execute Command
  if (!desc.handler) {
    return false;
  }
  bool success = desc.handler(argv, g);

  // 6. Deduct Fixed AP on success
  if (success && desc.ap.amount > 0) {
    if (desc.ap.model == APModel::FixedStar) {
      auto star_handle = g.entity_manager.get_star(g.snum());
      star_handle->AP(g.player()) -= desc.ap.amount;
    } else if (desc.ap.model == APModel::FixedUniv) {
      auto univ_handle = g.entity_manager.get_universe();
      univ_handle->AP[g.player().value - 1] -= desc.ap.amount;
    }
  }

  return success;
}

}  // namespace GB::commands
