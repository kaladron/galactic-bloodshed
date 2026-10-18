// SPDX-License-Identifier: Apache-2.0

/// \file launch.cc
/// \brief Launch a landed or docked ship.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.navigation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool launch(const command_t& argv, GameObj& g) {
  bool any_launched = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res =
        launch_single_ship(g.entity_manager, *ship_handle, g.player(), g.god());
    if (!res) {
      g.out << GB::presentation::format_launch_error(res.error());
      continue;
    }

    if (!res->observers.empty()) {
      const std::string observed =
          GB::presentation::render_launch_observed_telegram(*res);
      for (const auto& obs : res->observers) {
        warn_player(g.session_registry, g.entity_manager, obs.player,
                    obs.governor, observed);
      }
    }

    g.present(*res);
    any_launched = true;
  }

  return any_launched;
}

const std::array<std::string_view, 1> launch_aliases = {"undock"};

const CommandDescriptor launch_cmd{
    .name = "launch",
    .aliases = launch_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "launch <ship>",
    .description = "Launch a landed or docked ship",
    .handler = &launch,
};

}  // namespace GB::commands
