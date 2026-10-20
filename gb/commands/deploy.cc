// SPDX-License-Identifier: Apache-2.0

/// \file deploy.cc
/// \brief Deploy military troops across planetary sectors.

module;

import gb.entities;
import gb.services;

module commands;

namespace GB::commands {

bool deploy(const command_t& argv, GameObj& g) {
  return dispatch_planet_sector_movement(argv, g, PopulationType::MIL);
}

const CommandDescriptor deploy_cmd{
    .name = "deploy",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "deploy <from_sector> <path> [<amount>]",
    .description = "Deploy military troops across planetary sectors",
    .handler = &deploy,
};

}  // namespace GB::commands
