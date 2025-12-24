// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

#include "gb/GB_server.h"

module commands;

namespace {
void show_votes(GameObj& g) {
  int nvotes = 0;
  int nays = 0;
  int yays = 0;

  for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
    const auto* race = g.entity_manager.peek_race(i);
    if (!race || race->God || race->Guest) continue;
    nvotes++;
    if (race->votes) {
      yays++;
      if (g.god) g.out << std::format("  {0} voted go.\n", race->name);
    } else {
      nays++;
      if (g.god) g.out << std::format("  {0} voted wait.\n", race->name);
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
  for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
    const auto* r = g.entity_manager.peek_race(i);
    if (!r || r->God || r->Guest) continue;
    nvotes++;
    if (r->votes) {
      yays++;
    } else {
      nays++;
    }
  }
  /* Is Update/Movement vote unanimous now? */
  if (nvotes > 0 && nvotes == yays && nays == 0) {
    /* Do it... */
    do_next_thing(g.entity_manager);
  }
}
}  // namespace

namespace GB::commands {
void vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;

  auto race = g.entity_manager.get_race(Playernum);

  if (g.god) {
    g.out << "Your vote doesn't count, however, here is the count.\n";
    show_votes(g);
    return;
  }

  if (race->Guest) {
    g.out << "You are not allowed to vote, but, here is the count.\n";
    show_votes(g);
    return;
  }

  if (argv.size() <= 2) {
    g.out << std::format("Your vote on updates is {0}\n",
                         race->votes ? "go" : "wait");
    show_votes(g);
    return;
  }

  bool check = false;
  if (argv[1] != "update") {
    g.out << std::format("No such vote '{0}'\n", argv[1].c_str());
    return;
  }

  if (argv[2] == "go") {
    race->votes = true;
    check = true;
  } else if (argv[2] == "wait")
    race->votes = false;
  else {
    g.out << std::format("No such update choice '{0}'\n", argv[2].c_str());
    return;
  }

  if (check) check_votes(g);
}
}  // namespace GB::commands