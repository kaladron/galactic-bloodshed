// SPDX-License-Identifier: Apache-2.0

/// \file pledge.cc
/// \brief Pledge to join an alliance block.

module;

import std;
import gblib;
import notification;
import session;

module commands;

namespace GB::commands {
/* declare that you wish to be included in the alliance block */
bool pledge(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  player_t n = get_player(g.entity_manager, argv[1]);
  if (n == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  if (n == Playernum) {
    g.out << "Not needed, you are the leader.\n";
    return false;
  }

  const auto* race = g.entity_manager.peek_race(Playernum);
  if (!race) {
    g.out << "Race not found.\n";
    return false;
  }

  auto block_handle = g.entity_manager.get_block(n.value);
  if (!block_handle.get()) {
    g.out << "Block not found.\n";
    return false;
  }
  auto& block = *block_handle;

  setbit(block.pledge, Playernum);
  warn_race(g.session_registry, g.entity_manager, n,
            std::format("{} [{}] has pledged {}.\n", g.race->name, Playernum,
                        block.name));
  warn_race(g.session_registry, g.entity_manager, Playernum,
            std::format("You have pledged allegiance to {}.\n", block.name));

  std::string msg;
  switch (int_rand(1, 20)) {
    case 1:
      msg = std::format(
          "{} [{}] joins the band wagon and pledges allegiance to {} [{}]!\n",
          race->name, Playernum, block.name, n);
      break;
    default:
      msg = std::format("{} [{}] pledges allegiance to {} [{}].\n", race->name,
                        Playernum, block.name, n);
      break;
  }

  post(g.entity_manager, msg, NewsType::DECLARATION);
  compute_power_blocks(g.entity_manager);
  return true;
}

const CommandDescriptor pledge_cmd{
    .name = "pledge",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "pledge <race>",
    .description = "Pledge allegiance to join an alliance block",
    .handler = &pledge,
};

}  // namespace GB::commands