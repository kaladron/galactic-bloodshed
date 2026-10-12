// SPDX-License-Identifier: Apache-2.0

/// \file victory.cc
/// \brief Display current victory standings and player rankings.

module commands;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

namespace GB::commands {
bool victory(const command_t& argv, GameObj& g) {
  int count = g.entity_manager.num_races().value;
  if (argv.size() > 1) {
    auto scan_res = scn::scan<int>(argv[1], "{}");
    if (!scan_res || scan_res->value() < 1) {
      g.out << "Invalid count specified.\n";
      return false;
    }
    count = scan_res->value();
  }
  count = std::min(count, static_cast<int>(g.entity_manager.num_races().value));

  auto viclist = create_victory_list(g.entity_manager);

  GB::presentation::VictoryViewModel vm{
      .is_god = g.god(),
  };

  for (int rank = 0; const auto& vic : viclist) {
    rank++;
    if (rank > count) break;

    GB::presentation::VictoryRow row{
        .rank = rank,
        .is_metamorph = vic.thing != 0,
        .player = vic.racenum,
        .name = vic.name,
        .raw_score = vic.rawscore,
        .tech = vic.tech,
        .iq = vic.iq,
    };

    if (g.god()) {
      const auto* race = g.entity_manager.peek_race(vic.racenum);
      if (!race) continue;
      row.password = race->password;
      row.gov_password = race->leader().password;
    }

    vm.rows.push_back(std::move(row));
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor victory_cmd{
    .name = "victory",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "victory [<count>]",
    .description = "Display current victory standings and player rankings",
    .handler = &victory,
};

}  // namespace GB::commands
