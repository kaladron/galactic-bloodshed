// SPDX-License-Identifier: Apache-2.0

/// \file send_message.cc
/// \brief Send telegrams, stargrams, or alliance block messages.

module;

import gb.entities;
import gb.services;
import notification;
import session;
import std;

module commands;

namespace GB::commands {

bool send_message(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  bool postit = argv[0] == "post";
  ap_t APcount = postit || g.god() ? 0 : 1;
  player_t who = 0;
  starnum_t star = 0;
  bool to_block = false;
  bool to_star = false;
  std::string msg;

  if (postit) {
    const auto& race = *g.race;
    msg = std::format("{} \"{}\" [{},{}]: ", race.name,
                      race.governor[Governor.value].name, Playernum, Governor);
    for (auto j = 1U; j < argv.size(); ++j) {
      msg += argv[j] + " ";
    }
    msg += "\n";
    post(g.entity_manager, msg, NewsType::ANNOUNCE);
    return true;
  }

  if (argv[1] == "block") {
    if (argv.size() < 3) {
      g.out << "Syntax: send block <block> <message>\n";
      return false;
    }
    to_block = true;
    g.out << "Sending message to alliance block.\n";
    who = get_player(g.entity_manager, argv[2]);
    if (who == player_t{0}) {
      g.out << "No such alliance block.\n";
      return false;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Alien race not found.\n";
      return false;
    }
    APcount *= !alien->God;
  } else if (argv[1] == "star") {
    if (argv.size() < 3) {
      g.out << "Syntax: send star <star> <message>\n";
      return false;
    }
    to_star = true;
    g.out << "Sending message to star system.\n";
    Place where{g, argv[2], true};
    if (where.err || where.level != ScopeLevel::LEVEL_STAR) {
      g.out << "No such star.\n";
      return false;
    }
    star = where.snum;
  } else {
    who = get_player(g.entity_manager, argv[1]);
    if (who == player_t{0}) {
      g.out << "No such player.\n";
      return false;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Alien race not found.\n";
      return false;
    }
    APcount *= !alien->God;
  }

  // Telegrams sent to yourself are free of action point cost.
  if (who == Playernum) {
    APcount = 0;
  }

  if (APcount > 0) {
    if (!g.deduct_ap(g.snum(), APcount)) {
      g.out << std::format("You don't have {} action points there.\n", APcount);
      return false;
    }
  }

  const auto& race = *g.race;

  /* send the message */
  const struct block* block_target = nullptr;
  if (to_block) {
    try {
      block_target = g.entity_manager.peek_block(who.value);
    } catch (const EntityNotFoundError&) {
      g.out << "Block not found.\n";
      return false;
    }
    msg = std::format("{} \"{}\" [{},{}] to {} [{}]: ", race.name,
                      race.governor[Governor.value].name, Playernum, Governor,
                      block_target->name, who);
  } else if (to_star) {
    const auto& star_ref = *g.entity_manager.peek_star(star);
    msg = std::format("{} \"{}\" [{},{}] to inhabitants of {}: ", race.name,
                      race.governor[Governor.value].name, Playernum, Governor,
                      star_ref.get_name());
  } else {
    msg = std::format("{} \"{}\" [{},{}]: ", race.name,
                      race.governor[Governor.value].name, Playernum, Governor);
  }

  std::size_t start;
  if (to_star || to_block || std::isdigit(*argv[2].c_str()))
    start = 3;
  else
    start = 2;

  for (auto j = start; j < argv.size(); ++j) {
    msg += argv[j] + " ";
  }

  const auto notice = std::format(
      "{} \"{}\" [{},{}] has sent you a telegram. Use `read' to read it.\n",
      race.name, race.governor[Governor.value].name, Playernum, Governor);

  if (to_block) {
    const auto block_msg = std::format(
        "{} \"{}\" [{},{}] sends a message to {} [{}] alliance block.\n",
        race.name, race.governor[Governor.value].name, Playernum, Governor,
        block_target->name, who);
    for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
      if (block_target->is_invited(i) && block_target->is_pledged(i) &&
          i != Playernum) {
        g.entity_manager.mutate_race(i, [&](Race& alien) {
          alien.translate[Playernum] =
              std::min(alien.translate[Playernum] + 2, 100);
        });
        g.session_registry.notify_race(i, block_msg);
        g.session_registry.notify_race(i, notice);
        push_telegram(g.entity_manager, i, 0, msg);
      }
    }
  } else if (to_star) {
    const auto& star_ref = *g.entity_manager.peek_star(star);
    for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
      if (star_ref.is_inhabited_by(i) && i != Playernum) {
        g.entity_manager.mutate_race(i, [&](Race& alien) {
          alien.translate[Playernum] =
              std::min(alien.translate[Playernum] + 2, 100);
        });
        g.session_registry.notify_race(
            i, std::format("{} \"{}\" [{},{}] sends a message to {}.\n",
                           race.name, race.governor[Governor.value].name,
                           Playernum, Governor, star_ref.get_name()));
        g.session_registry.notify_race(i, notice);
        push_telegram(g.entity_manager, i, 0, msg);
      }
    }
  } else {
    g.entity_manager.mutate_race(who, [&](Race& alien) {
      alien.translate[Playernum] =
          std::min(alien.translate[Playernum] + 2, 100);
    });
    int gov;
    if (std::isdigit(*argv[2].c_str()))
      gov = std::stoi(argv[2]);
    else
      gov = 0;
    if (gov != 0) {
      g.session_registry.notify_player(
          who, governor_t{static_cast<unsigned char>(gov)}, notice);
    } else {
      g.session_registry.notify_race(who, notice);
    }
    push_telegram(g.entity_manager, who, gov, msg);
  }

  g.out << "Message sent.\n";
  return true;
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
