// SPDX-License-Identifier: Apache-2.0

/// \file personal.cc
/// \brief Set personal description info for your race.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool personal(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  g.entity_manager.mutate_race(Playernum,
                               [&](Race& race) { race.info = message; });
  g.out << "Personal information updated.\n";
  return true;
}

const CommandDescriptor personal_cmd{
    .name = "personal",
    .roles = {.leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "personal <info text>",
    .description = "Set personal information description for your race",
    .handler = &personal,
};

}  // namespace GB::commands
