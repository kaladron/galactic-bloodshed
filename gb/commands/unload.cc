// SPDX-License-Identifier: Apache-2.0

/// \file unload.cc
/// \brief Unload commodities from ships onto planets or docked vessels.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool unload(const command_t& argv, GameObj& g) {
  if (argv[2].empty()) {
    g.out << "Unload what?\n";
    return false;
  }

  std::int64_t requested_amt = 0;
  if (argv.size() > 3) {
    auto parsed_amt = scn::scan<std::int64_t>(argv[3], "{}");
    if (!parsed_amt) {
      g.out << "Invalid amount.\n";
      return false;
    }
    requested_amt = parsed_amt->value();
  }

  const char commod = argv[2][0];
  bool success = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    bool should_return_early = false;
    if (dispatch_cargo_transfer(*ship_handle, commod, -requested_amt, true, g,
                                should_return_early)) {
      success = true;
      if (should_return_early) {
        return true;
      }
    }
  }
  return success;
}

const CommandDescriptor unload_cmd{
    .name = "unload",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "unload <ship> <commodity> [<amount>]",
    .description = "Unload commodities from a ship",
    .handler = &unload,
};

}  // namespace GB::commands
