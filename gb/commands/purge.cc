// SPDX-License-Identifier: Apache-2.0

/// \file purge.cc
/// \brief Purge all galactic news items.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool purge(const command_t&, GameObj& g) {
  ::purge(g.entity_manager);
  g.out << "Purged all news.\n";
  return true;
}

const CommandDescriptor purge_cmd{
    .name = "purge",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .description = "Purge all galactic news items (deity only)",
    .handler = &purge,
};

}  // namespace GB::commands
