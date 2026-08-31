// SPDX-License-Identifier: Apache-2.0

/// \file vote.cc
/// \brief Cast or inspect votes for universe turn updates.

module;

import gb.entities;
import gb.services;
import std;
import notification;

module commands;

namespace {
void show_votes(GameObj& g) {
  int nvotes = 0;
  int nays = 0;
  int yays = 0;

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    if (race.God || race.Guest) continue;
    nvotes++;
    if (race.votes) {
      yays++;
      if (g.god()) g.out << std::format("  {0} voted go.\n", race.name);
    } else {
      nays++;
      if (g.god()) g.out << std::format("  {0} voted wait.\n", race.name);
    }
  }
  g.out << std::format("  Total votes = {0}, Go = {1}, Wait = {2}.\n", nvotes,
                       yays, nays);
}

/**
 * @brief Tally votes and determine if the update or moveseg should be taken.
 *
 * This function iterates through all races and counts the number of "yes" and
 * "no" votes, excluding votes from God and Guest races. If all votes are "yes"
 * and there are no "no" votes, it triggers the next action.
 *
 * @param g Reference to the GameObj which contains the game state and database.
 */
void check_votes(GameObj& g) {
  // Ok...someone voted yes.  Tally them all up and see if we should do
  // something.
  int nays = 0;
  int yays = 0;
  int nvotes = 0;
  for (const Race& r : RaceList::readonly(g.entity_manager)) {
    if (r.God || r.Guest) continue;
    nvotes++;
    if (r.votes) {
      yays++;
    } else {
      nays++;
    }
  }
  /* Is Update/Movement vote unanimous now? */
  if (nvotes > 0 && nvotes == yays && nays == 0) {
    /* Signal server to execute next step after command completes */
    g.session_registry.request_next_thing();
  }
}
}  // namespace

namespace GB::commands {

bool vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();

  if (g.god()) {
    g.out << "Your vote doesn't count, however, here is the count.\n";
    show_votes(g);
    return true;
  }

  if (g.race->Guest) {
    g.out << "You are not allowed to vote, but, here is the count.\n";
    show_votes(g);
    return true;
  }

  if (argv.size() <= 2) {
    g.out << std::format("Your vote on updates is {0}\n",
                         g.race->votes ? "go" : "wait");
    show_votes(g);
    return true;
  }

  bool check = false;
  bool new_vote = false;
  if (argv[1] != "update") {
    g.out << std::format("No such vote '{0}'\n", argv[1].c_str());
    return false;
  }

  if (argv[2] == "go") {
    new_vote = true;
    check = true;
  } else if (argv[2] == "wait") {
    new_vote = false;
  } else {
    g.out << std::format("No such update choice '{0}'\n", argv[2].c_str());
    return false;
  }

  g.entity_manager.mutate_race(Playernum,
                               [&](Race& race) { race.votes = new_vote; });

  if (check) check_votes(g);
  return true;
}

const CommandDescriptor vote_cmd{
    .name = "vote",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "vote [update <go|wait>]",
    .description = "Cast or inspect votes for universe turn updates",
    .handler = &vote,
};

}  // namespace GB::commands