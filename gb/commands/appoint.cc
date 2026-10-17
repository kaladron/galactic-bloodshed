// SPDX-License-Identifier: Apache-2.0

/// \file appoint.cc
/// \brief Appoint race governors command.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool appoint(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  if (argv.size() == 2) {
    governor_t gov{0};
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      gov = race.appoint_governor({.password = argv[1]});
    });
    g.present(GB::presentation::GovernorAppointedResult{
        .governor = gov,
        .auto_assigned = true,
    });
    return true;
  }

  const auto parsed_gov = scn::scan<governor_t::value_type>(argv[1], "{}");
  if (!parsed_gov || parsed_gov->value() < 1) {
    g.out << "No such governor.\n";
    return false;
  }
  const governor_t gov{parsed_gov->value()};

  bool success = false;
  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    if (Race::is_leader(gov) || race.has_governor(gov)) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    race.appoint_governor(gov, {.password = argv[2]});
    success = true;
  });
  if (success) {
    g.present(GB::presentation::GovernorAppointedResult{
        .governor = gov,
        .auto_assigned = false,
    });
  }
  return success;
}

const CommandDescriptor appoint_cmd{
    .name = "appoint",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "appoint [<gov>] <password>",
    .description = "Appoint a new race governor",
    .handler = &appoint,
};

}  // namespace GB::commands
