// SPDX-License-Identifier: Apache-2.0

/// \file victory.cc
/// \brief Display current victory standings and player rankings.

module commands;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

namespace GB::commands {
bool victory(const command_t& argv, GameObj& g) {
  std::optional<int> requested_count;
  if (argv.size() > 1) {
    auto scan_res = parse_int<int>(argv[1]);
    if (!scan_res || *scan_res < 1) {
      g.out << "Invalid count specified.\n";
      return false;
    }
    requested_count = *scan_res;
  }

  auto viclist = create_victory_list(g.entity_manager);
  const int total = static_cast<int>(viclist.size());
  const int count = requested_count ? std::min(*requested_count, total) : total;

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
    vm.races.push_back(GB::presentation::project_race_view(
        g.entity_manager, g.scope_context(), vic.racenum, rank));
  }

  g.present(vm);
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
