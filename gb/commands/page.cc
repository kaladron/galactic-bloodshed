// SPDX-License-Identifier: Apache-2.0

/// \file page.cc
/// \brief Page a player or alliance block.

module;

import gblib;
import notification;
import session;
import std;

module commands;

namespace GB::commands {

bool page(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = g.god() ? 0 : 1;
  player_t who = 0;
  governor_t gov{0};
  bool to_block = false;

  if (argv[1] == "block") {
    to_block = true;
    g.out << "Paging alliance block.\n";
  } else {
    who = get_player(g.entity_manager, argv[1]);
    if (who == player_t{0}) {
      g.out << "No such player.\n";
      return false;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Race not found.\n";
      return false;
    }
    APcount *= !alien->God;
    if (argv.size() > 2) {
      gov = governor_t{static_cast<unsigned char>(std::stoi(argv[2]))};
    }
  }

  if (APcount > 0) {
    if (!g.deduct_ap(g.snum(), APcount)) {
      g.out << std::format("You don't have {} action points there.\n", APcount);
      return false;
    }
  }

  const auto& star = *g.entity_manager.peek_star(g.snum());
  auto msg = std::format("{} \"{}\" page(s) you from the {} star system.\n",
                         g.race->name, g.race->governor[Governor.value].name,
                         star.get_name());

  if (to_block) {
    const auto* block_player = g.entity_manager.peek_block(Playernum.value);
    if (!block_player) {
      g.out << "Block not found.\n";
      return false;
    }
    std::uint64_t allied_members = block_player->invite & block_player->pledge;
    for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
      if (isset(allied_members, i) && i != Playernum) {
        g.session_registry.notify_race(i, msg);
      }
    }
  } else {
    if (argv.size() > 2) {
      g.session_registry.notify_player(who, gov, msg);
    } else {
      g.session_registry.notify_race(who, msg);
    }
  }

  g.out << "Request sent.\n";
  return true;
}

const CommandDescriptor page_cmd{
    .name = "page",
    .roles = {},
    .scopes = {.star = true, .planet = true, .ship = true},
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "page <race|block> [<governor>]",
    .description = "Page a player or alliance block",
    .handler = &page,
};

}  // namespace GB::commands
