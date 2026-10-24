// SPDX-License-Identifier: Apache-2.0

/// \file page.cc
/// \brief Page a player or alliance block.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

bool deduct_page_ap(GameObj& g, ap_t ap_cost) {
  if (ap_cost == 0) {
    return true;
  }
  if (!g.deduct_ap(g.snum(), ap_cost)) {
    g.out << std::format("You don't have {} action points there.\n", ap_cost);
    return false;
  }
  return true;
}

std::expected<std::optional<governor_t>, bool>
parse_page_governor(const command_t& argv, const Race& alien, GameObj& g) {
  if (argv.size() <= 2) {
    return std::optional<governor_t>{std::nullopt};
  }
  auto parsed_gov = scn::scan<int>(argv[2], "{}");
  if (!parsed_gov || parsed_gov->value() < 1 ||
      !alien.has_governor(governor_t{parsed_gov->value()})) {
    g.out << "No such governor.\n";
    return std::unexpected(false);
  }
  return std::optional<governor_t>{governor_t{parsed_gov->value()}};
}

bool page_alliance_block(GameObj& g) {
  const ap_t ap_cost = g.god() ? 0 : 1;
  if (!deduct_page_ap(g, ap_cost)) {
    return false;
  }

  const auto playernum = g.player();
  const auto& star = *g.entity_manager.peek_star(g.snum());
  const auto msg = GB::presentation::render_page_notification(
      g.race->name, g.current_governor().name, star.get_name());
  const auto& block_player = *g.entity_manager.peek_block(playernum);

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const auto i = race.Playernum;
    if (block_player.is_member(i) && i != playernum) {
      g.session_registry.notify_race(i, msg);
    }
  }

  g.present(GB::presentation::PageResult{
      .target = GB::presentation::PageTarget::Block,
  });
  return true;
}

bool page_player(const command_t& argv, GameObj& g) {
  const auto who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  const auto& alien = *g.entity_manager.peek_race(who);
  const auto target_gov = parse_page_governor(argv, alien, g);
  if (!target_gov) {
    return false;
  }

  const ap_t ap_cost = (g.god() || alien.God) ? 0 : 1;
  if (!deduct_page_ap(g, ap_cost)) {
    return false;
  }

  const auto& star = *g.entity_manager.peek_star(g.snum());
  const auto msg = GB::presentation::render_page_notification(
      g.race->name, g.current_governor().name, star.get_name());
  if (*target_gov) {
    g.session_registry.notify_player(who, **target_gov, msg);
  } else {
    g.session_registry.notify_race(who, msg);
  }

  g.present(GB::presentation::PageResult{
      .target = GB::presentation::PageTarget::Player,
  });
  return true;
}

}  // namespace

namespace GB::commands {

bool page(const command_t& argv, GameObj& g) {
  if (argv[1] == "block") {
    return page_alliance_block(g);
  }
  return page_player(argv, g);
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
