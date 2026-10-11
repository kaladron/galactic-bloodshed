// SPDX-License-Identifier: Apache-2.0

/// \file relation.cc
/// \brief Display relations among players.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

module commands;

static std::string allied(const Race& r, const player_t p) {
  if (r.is_at_war_with(p)) return "WAR";
  if (r.is_allied_with(p)) return "ALLIED";
  return "neutral";
}

namespace GB::commands {
bool relation(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  player_t q = Playernum;
  if (argv.size() > 1) {
    q = get_player(g.entity_manager, argv[1]);
    if (q == player_t{0}) {
      g.out << "No such player.\n";
      return false;
    }
  }

  try {
    g.entity_manager.with_race(q, [&](const Race& race) {
      GB::presentation::RelationViewModel vm{
          .subject_player = race.Playernum,
          .subject_race_name = race.name,
      };
      for (const Race& r : RaceList::readonly(g.entity_manager)) {
        if (r.Playernum == race.Playernum) continue;
        const Percentage know = race.translation_for(r);
        vm.rows.push_back(GB::presentation::RelationRow{
            .player = r.Playernum,
            .is_known_morph = (know > 30 && r.Metamorph && Playernum == q),
            .know = know,
            .race_name = r.name,
            .yours = allied(race, r.Playernum),
            .theirs = allied(r, q),
        });
      }
      g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Race not found.\n";
    return false;
  }
  return true;
}

const CommandDescriptor relation_cmd{
    .name = "relation",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "relation [<race>]",
    .description =
        "Display diplomatic relations and mutual standing between races",
    .handler = &relation,
};

}  // namespace GB::commands
