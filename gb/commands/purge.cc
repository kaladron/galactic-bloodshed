// SPDX-License-Identifier: Apache-2.0

/// \file purge.cc
/// \brief Purge all galactic news items.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool purge(const command_t& argv, GameObj& g) {
  if (argv.size() > 1) {
    g.out << "Syntax: purge\n";
    return false;
  }
  ::purge(g.entity_manager);
  g.present(GB::presentation::PurgeResult{});
  return true;
}

const CommandDescriptor purge_cmd{
    .name = "purge",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "purge",
    .description = "Purge all galactic news items (deity only)",
    .handler = &purge,
};

}  // namespace GB::commands
