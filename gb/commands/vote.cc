// SPDX-License-Identifier: Apache-2.0

/// \file vote.cc
/// \brief Cast or inspect votes for universe turn updates.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {

GB::presentation::VoteViewModel
tally_votes(GameObj& g, GB::presentation::VoteViewerRole role) {
  GB::presentation::VoteViewModel vm{
      .viewer_role = role,
      .own_vote_go = g.race->votes,
  };

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    if (race.God || race.Guest) continue;
    vm.total_votes++;
    if (race.votes) {
      vm.yays++;
    } else {
      vm.nays++;
    }
    if (g.god()) {
      vm.details.push_back(GB::presentation::RaceVoteEntry{
          .race_name = race.name,
          .vote_go = race.votes,
      });
    }
  }
  return vm;
}

}  // namespace

namespace GB::commands {

bool vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();

  if (g.god()) {
    g.present(tally_votes(g, GB::presentation::VoteViewerRole::God));
    return true;
  }

  if (g.race->Guest) {
    g.present(tally_votes(g, GB::presentation::VoteViewerRole::Guest));
    return true;
  }

  if (argv.size() <= 2) {
    g.present(tally_votes(g, GB::presentation::VoteViewerRole::Mortal));
    return true;
  }

  if (argv[1] != "update") {
    g.out << std::format("No such vote '{}'\n", argv[1]);
    return false;
  }

  bool new_vote = false;
  if (argv[2] == "go") {
    new_vote = true;
  } else if (argv[2] == "wait") {
    new_vote = false;
  } else {
    g.out << std::format("No such update choice '{}'\n", argv[2]);
    return false;
  }

  g.entity_manager.mutate_race(Playernum,
                               [&](Race& race) { race.votes = new_vote; });

  bool unanimous = false;
  if (new_vote) {
    const auto tally = tally_votes(g, GB::presentation::VoteViewerRole::Mortal);
    if (tally.total_votes > 0 && tally.nays == 0) {
      g.session_registry.request_next_thing();
      unanimous = true;
    }
  }

  g.present(GB::presentation::VoteCastResult{
      .player = Playernum,
      .vote_go = new_vote,
      .unanimous = unanimous,
  });
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