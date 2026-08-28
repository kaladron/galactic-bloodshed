// SPDX-License-Identifier: Apache-2.0

/// \file relation.cc
/// \brief Display relations among players.

module;

import std;
import gb.entities;
import gb.services;

module commands;

static auto allied(const Race& r, const player_t p) {
  if (isset(r.atwar, p)) return "WAR";
  if (isset(r.allied, p)) return "ALLIED";
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
      g.out << std::format("\n              Racial Relations Report for {}\n\n",
                           race.name);
      g.out
          << " #       know             Race name       Yours        Theirs\n";
      g.out
          << " -       ----             ---------       -----        ------\n";
      for (const Race& r : RaceList::readonly(g.entity_manager)) {
        if (r.Playernum == race.Playernum) continue;
        g.out << std::format(
            "{:2} {:5} ({:3d}%) {:>20.20} : {:>10}   {:>10}\n", r.Playernum,
            ((race.God || (race.translate[r.Playernum.value - 1] > 30)) &&
             r.Metamorph && (Playernum == q))
                ? "Morph"
                : "     ",
            race.translate[r.Playernum.value - 1], r.name,
            allied(race, r.Playernum), allied(r, q));
      }
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
