// SPDX-License-Identifier: Apache-2.0

/// \file send_message.cc
/// \brief Send telegrams, stargrams, or alliance block messages.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

/**
 * @brief Join command tokens from `start` into a space-delimited message body.
 */
std::string join_message_tokens(const command_t& argv, std::size_t start) {
  return argv | std::views::drop(start) | std::views::join_with(' ') |
         std::ranges::to<std::string>();
}

/**
 * @brief Deduct dynamic AP cost for sending a message if `ap_cost > 0`.
 */
bool deduct_message_ap(GameObj& g, ap_t ap_cost) {
  if (ap_cost == 0) {
    return true;
  }
  if (!g.deduct_ap(g.snum(), ap_cost)) {
    g.out << std::format("You don't have {} action points there.\n", ap_cost);
    return false;
  }
  return true;
}

/**
 * @brief Increment recipient race's translation skill toward `sender` by 2%
 * (capped at 100%).
 */
void increment_translation_skill(EntityManager& em, player_t recipient,
                                 player_t sender) {
  em.mutate_race(recipient,
                 [&](Race& alien) { alien.increase_translation(sender, 2); });
}

struct PlayerMessageRoute {
  std::optional<governor_t> target_gov{std::nullopt};
  std::size_t body_start{2};
};

std::optional<PlayerMessageRoute>
parse_player_message_route(const command_t& argv, const Race& alien,
                           GameObj& g) {
  if (argv[2].empty() ||
      !std::isdigit(static_cast<unsigned char>(argv[2][0]))) {
    return PlayerMessageRoute{};
  }
  if (argv.size() < 4) {
    g.out << "Syntax: send <race> [<governor>] <message>\n";
    return std::nullopt;
  }
  auto parsed_gov = scn::scan<int>(argv[2], "{}");
  if (!parsed_gov || parsed_gov->value() < 1 ||
      !alien.has_governor(governor_t{parsed_gov->value()})) {
    g.out << "No such governor.\n";
    return std::nullopt;
  }
  return PlayerMessageRoute{
      .target_gov = governor_t{parsed_gov->value()},
      .body_start = 3,
  };
}

/**
 * @brief Handle `send block <block> <message>`.
 */
bool send_to_alliance_block(const command_t& argv, GameObj& g) {
  if (argv.size() < 4) {
    g.out << "Syntax: send block <block> <message>\n";
    return false;
  }
  const auto who = get_player(g.entity_manager, argv[2]);
  if (who == player_t{0}) {
    g.out << "No such alliance block.\n";
    return false;
  }

  const auto& block_target = *g.entity_manager.peek_block(who);

  const auto playernum = g.player();
  const ap_t ap_cost = g.god() ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const auto sender_tag =
      GB::presentation::format_sender_tag(*g.race, playernum, g.governor());
  const auto msg =
      std::format("{} to {} [{}]: {}", sender_tag, block_target.name(), who,
                  join_message_tokens(argv, 3));
  const auto notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);
  const auto block_msg =
      std::format("{} sends a message to {} [{}] alliance block.\n", sender_tag,
                  block_target.name(), who);

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const auto i = race.Playernum;
    if (block_target.is_member(i) && i != playernum) {
      increment_translation_skill(g.entity_manager, i, playernum);
      g.session_registry.notify_race(i, block_msg);
      g.session_registry.notify_race(i, notice);
      push_telegram(g.entity_manager, i, Race::leader_id, msg);
    }
  }

  g.present(GB::presentation::SendMessageResult{
      .target = GB::presentation::SendMessageTarget::Block,
  });
  return true;
}

/**
 * @brief Handle `send star <star> <message>`.
 */
bool send_to_star_system(const command_t& argv, GameObj& g) {
  if (argv.size() < 4) {
    g.out << "Syntax: send star <star> <message>\n";
    return false;
  }
  auto where =
      Place::resolve(g.entity_manager, g.scope_context(), argv[2], true);
  if (!where) {
    g.out << format_place_error(where.error());
    g.out << "No such star.\n";
    return false;
  }
  if (where->level != ScopeLevel::LEVEL_STAR) {
    g.out << "No such star.\n";
    return false;
  }

  const ap_t ap_cost = g.god() ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const auto playernum = g.player();
  const auto& star_ref = *g.entity_manager.peek_star(where->snum);
  const auto sender_tag =
      GB::presentation::format_sender_tag(*g.race, playernum, g.governor());
  const auto msg =
      std::format("{} to inhabitants of {}: {}", sender_tag,
                  star_ref.get_name(), join_message_tokens(argv, 3));
  const auto notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);
  const auto star_msg = std::format("{} sends a message to {}.\n", sender_tag,
                                    star_ref.get_name());

  for (const Race& race : RaceList::readonly(g.entity_manager)) {
    const auto i = race.Playernum;
    if (star_ref.is_inhabited_by(i) && i != playernum) {
      increment_translation_skill(g.entity_manager, i, playernum);
      g.session_registry.notify_race(i, star_msg);
      g.session_registry.notify_race(i, notice);
      push_telegram(g.entity_manager, i, Race::leader_id, msg);
    }
  }

  g.present(GB::presentation::SendMessageResult{
      .target = GB::presentation::SendMessageTarget::Star,
  });
  return true;
}

/**
 * @brief Handle `send <race> [<governor>] <message>`.
 */
bool send_to_player(const command_t& argv, GameObj& g) {
  const auto who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  const auto& alien = *g.entity_manager.peek_race(who);
  const auto route = parse_player_message_route(argv, alien, g);
  if (!route) {
    return false;
  }

  const auto playernum = g.player();
  const ap_t ap_cost = (g.god() || who == playernum || alien.God) ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const auto sender_tag =
      GB::presentation::format_sender_tag(*g.race, playernum, g.governor());
  const auto msg = std::format("{}: {}", sender_tag,
                               join_message_tokens(argv, route->body_start));
  const auto notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);

  increment_translation_skill(g.entity_manager, who, playernum);
  if (route->target_gov) {
    g.session_registry.notify_player(who, *route->target_gov, notice);
  } else {
    g.session_registry.notify_race(who, notice);
  }
  push_telegram(g.entity_manager, who,
                route->target_gov.value_or(Race::leader_id), msg);

  g.present(GB::presentation::SendMessageResult{
      .target = GB::presentation::SendMessageTarget::Player,
  });
  return true;
}

}  // namespace

namespace GB::commands {

bool send_message(const command_t& argv, GameObj& g) {
  if (argv[1] == "block") {
    return send_to_alliance_block(argv, g);
  }
  if (argv[1] == "star") {
    return send_to_star_system(argv, g);
  }
  return send_to_player(argv, g);
}

const CommandDescriptor send_cmd{
    .name = "send",
    .roles = {},
    .scopes = {.star = true, .planet = true, .ship = true},
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "send <race|block|star> [<governor>] <message>",
    .description =
        "Send private telegrams, alliance block messages, or stargrams",
    .handler = &send_message,
};

}  // namespace GB::commands
