// SPDX-License-Identifier: Apache-2.0

/// \file send_message.cc
/// \brief Send telegrams, stargrams, or alliance block messages.

module;

import gb.entities;
import gb.services;
import notification;
import scnlib;
import session;
import std;

module commands;

namespace {

/**
 * @brief Format sender prefix (`<Race> "<Gov>" [<player>,<gov>]`).
 */
std::string format_sender_tag(const Race& race, player_t playernum,
                              governor_t governor) {
  return std::format("{} \"{}\" [{},{}]", race.name,
                     race.governor[governor.value].name, playernum, governor);
}

/**
 * @brief Join command tokens from `start` into a space-delimited message body.
 */
std::string join_message_tokens(const command_t& argv, std::size_t start) {
  std::string body;
  for (auto j = start; j < argv.size(); ++j) {
    body += argv[j] + " ";
  }
  return body;
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

/**
 * @brief Handle `post <message>` public bulletin announcement.
 */
bool handle_post_command(const command_t& argv, GameObj& g) {
  const auto& race = *g.race;
  std::string msg =
      std::format("{}: {}\n", format_sender_tag(race, g.player(), g.governor()),
                  join_message_tokens(argv, 1));
  post(g.entity_manager, msg, NewsType::ANNOUNCE);
  return true;
}

/**
 * @brief Handle `send block <block> <message>`.
 */
bool send_to_alliance_block(const command_t& argv, GameObj& g) {
  if (argv.size() < 4) {
    g.out << "Syntax: send block <block> <message>\n";
    return false;
  }
  g.out << "Sending message to alliance block.\n";
  const player_t who = get_player(g.entity_manager, argv[2]);
  if (who == player_t{0}) {
    g.out << "No such alliance block.\n";
    return false;
  }
  const auto* alien = g.entity_manager.peek_race(who);
  if (!alien) {
    g.out << "Alien race not found.\n";
    return false;
  }

  const block* block_target = nullptr;
  try {
    block_target = g.entity_manager.peek_block(who.value);
  } catch (const EntityNotFoundError&) {
    g.out << "Block not found.\n";
    return false;
  }

  const player_t playernum = g.player();
  const ap_t ap_cost = g.god() ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const std::string sender_tag =
      format_sender_tag(*g.race, playernum, g.governor());
  const std::string msg =
      std::format("{} to {} [{}]: {}", sender_tag, block_target->name, who,
                  join_message_tokens(argv, 3));
  const std::string notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);
  const std::string block_msg =
      std::format("{} sends a message to {} [{}] alliance block.\n", sender_tag,
                  block_target->name, who);

  for (player_t i = 1; i <= g.entity_manager.num_races(); ++i) {
    if (block_target->is_member(i) && i != playernum) {
      increment_translation_skill(g.entity_manager, i, playernum);
      g.session_registry.notify_race(i, block_msg);
      g.session_registry.notify_race(i, notice);
      push_telegram(g.entity_manager, i, 0, msg);
    }
  }

  g.out << "Message sent.\n";
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
  g.out << "Sending message to star system.\n";
  Place where{g, argv[2], true};
  if (where.err || where.level != ScopeLevel::LEVEL_STAR) {
    g.out << "No such star.\n";
    return false;
  }

  const ap_t ap_cost = g.god() ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const player_t playernum = g.player();
  const auto& star_ref = *g.entity_manager.peek_star(where.snum);
  const std::string sender_tag =
      format_sender_tag(*g.race, playernum, g.governor());
  const std::string msg =
      std::format("{} to inhabitants of {}: {}", sender_tag,
                  star_ref.get_name(), join_message_tokens(argv, 3));
  const std::string notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);
  const std::string star_msg = std::format("{} sends a message to {}.\n",
                                           sender_tag, star_ref.get_name());

  for (player_t i = 1; i <= g.entity_manager.num_races(); ++i) {
    if (star_ref.is_inhabited_by(i) && i != playernum) {
      increment_translation_skill(g.entity_manager, i, playernum);
      g.session_registry.notify_race(i, star_msg);
      g.session_registry.notify_race(i, notice);
      push_telegram(g.entity_manager, i, 0, msg);
    }
  }

  g.out << "Message sent.\n";
  return true;
}

/**
 * @brief Handle `send <race> [<governor>] <message>`.
 */
bool send_to_player(const command_t& argv, GameObj& g) {
  const player_t who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  const auto* alien = g.entity_manager.peek_race(who);
  if (!alien) {
    g.out << "Alien race not found.\n";
    return false;
  }

  int gov = 0;
  std::size_t start = 2;
  if (std::isdigit(static_cast<unsigned char>(argv[2][0]))) {
    if (argv.size() < 4) {
      g.out << "Syntax: send <race> [<governor>] <message>\n";
      return false;
    }
    auto parsed_gov = scn::scan<int>(argv[2], "{}");
    if (!parsed_gov || parsed_gov->value() < 0 ||
        parsed_gov->value() > MAXGOVERNORS) {
      g.out << "No such governor.\n";
      return false;
    }
    gov = parsed_gov->value();
    start = 3;
  }

  const player_t playernum = g.player();
  const ap_t ap_cost = (g.god() || who == playernum || alien->God) ? 0 : 1;
  if (!deduct_message_ap(g, ap_cost)) {
    return false;
  }

  const std::string sender_tag =
      format_sender_tag(*g.race, playernum, g.governor());
  const std::string msg =
      std::format("{}: {}", sender_tag, join_message_tokens(argv, start));
  const std::string notice = std::format(
      "{} has sent you a telegram. Use `read' to read it.\n", sender_tag);

  increment_translation_skill(g.entity_manager, who, playernum);
  if (gov != 0) {
    g.session_registry.notify_player(
        who, governor_t{static_cast<unsigned char>(gov)}, notice);
  } else {
    g.session_registry.notify_race(who, notice);
  }
  push_telegram(g.entity_manager, who, gov, msg);

  g.out << "Message sent.\n";
  return true;
}

}  // namespace

namespace GB::commands {

bool send_message(const command_t& argv, GameObj& g) {
  if (argv[0] == "post") {
    return handle_post_command(argv, g);
  }
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

const CommandDescriptor post_cmd{
    .name = "post",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "post <message>",
    .description = "Post a public announcement bulletin to all players",
    .handler = &send_message,
};

}  // namespace GB::commands
