// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void send_message(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  bool postit = argv[0] == "post";
  ap_t APcount;
  if (postit) {
    APcount = 0;
  } else {
    APcount = g.god ? 0 : 1;
  }
  int who;
  player_t i;
  int j;
  int to_block;
  int to_star;
  int star;
  int start;
  std::string msg;

  star = 0;  // TODO(jeffbailey): Init to zero.
  who = 0;   // TODO(jeffbailey): Init to zero.

  to_star = to_block = 0;

  if (argv.size() < 2) {
    g.out << "Send what?\n";
    return;
  }
  if (postit) {
    const auto* race_ptr =
        g.race ? g.race : g.entity_manager.peek_race(Playernum);
    if (!race_ptr) {
      g.out << "Race not found.\n";
      return;
    }
    const auto& race = *race_ptr;
    msg = std::format("{} \"{}\" [{},{}]: ", race.name,
                      race.governor[Governor].name, Playernum, Governor);
    for (j = 1; j < argv.size(); j++)
      msg += argv[j] + " ";
    msg += "\n";
    post(g.entity_manager, msg, NewsType::ANNOUNCE);
    return;
  }
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Sending message to alliance block.\n";
    if (!(who = get_player(g.entity_manager, argv[2]))) {
      g.out << "No such alliance block.\n";
      return;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Alien race not found.\n";
      return;
    }
    APcount *= !alien->God;
  } else if (argv[1] == "star") {
    to_star = 1;
    g.out << "Sending message to star system.\n";
    Place where{g, argv[2], true};
    if (where.err || where.level != ScopeLevel::LEVEL_STAR) {
      g.out << "No such star.\n";
      return;
    }
    star = where.snum;
  } else {
    if (!(who = get_player(g.entity_manager, argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
    const auto* alien = g.entity_manager.peek_race(who);
    if (!alien) {
      g.out << "Alien race not found.\n";
      return;
    }
    APcount *= !alien->God;
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't send messages from universal scope.\n";
      return;

    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can't send messages from ship scope.\n";
      return;

    default:
      if (!enufAP(Playernum, Governor,
                  g.entity_manager.peek_star(g.snum)->AP(Playernum - 1),
                  APcount))
        return;
      break;
  }

  const auto* race_ptr =
      g.race ? g.race : g.entity_manager.peek_race(Playernum);
  if (!race_ptr) {
    g.out << "Race not found.\n";
    return;
  }
  const auto& race = *race_ptr;

  /* send the message */
  if (to_block) {
    const auto* block = g.entity_manager.peek_block(who);
    if (!block) {
      g.out << "Block not found.\n";
      return;
    }
    msg = std::format("{} \"{}\" [{},{}] to {} [{}]: ", race.name,
                      race.governor[Governor].name, Playernum, Governor,
                      block->name, who);
  } else if (to_star) {
    const auto& star_ref = *g.entity_manager.peek_star(star);
    msg = std::format("{} \"{}\" [{},{}] to inhabitants of {}: ", race.name,
                      race.governor[Governor].name, Playernum, Governor,
                      star_ref.get_name());
  } else {
    msg = std::format("{} \"{}\" [{},{}]: ", race.name,
                      race.governor[Governor].name, Playernum, Governor);
  }

  if (to_star || to_block || std::isdigit(*argv[2].c_str()))
    start = 3;
  else if (postit)
    start = 1;
  else
    start = 2;
  /* put the message together */
  for (j = start; j < argv.size(); j++)
    msg += argv[j] + " ";
  /* post it */
  const auto notice = std::format(
      "{} \"{}\" [{},{}] has sent you a telegram. Use `read' to read it.\n",
      race.name, race.governor[Governor].name, Playernum, Governor);
  if (to_block) {
    const auto* block = g.entity_manager.peek_block(who);
    if (!block) {
      g.out << "Block not found.\n";
      return;
    }
    std::uint64_t allied_members = (block->invite & block->pledge);
    const auto block_msg = std::format(
        "{} \"{}\" [{},{}] sends a message to {} [{}] alliance block.\n",
        race.name, race.governor[Governor].name, Playernum, Governor,
        block->name, who);
    for (i = 1; i <= g.entity_manager.num_races(); i++) {
      if (isset(allied_members, i)) {
        notify_race(i, block_msg);
        push_telegram_race(g.entity_manager, i, msg);
      }
    }
  } else if (to_star) {
    const auto& star_ref = *g.entity_manager.peek_star(star);
    const auto star_msg = std::format(
        "{} \"{}\" [{},{}] sends a stargram to {}.\n", race.name,
        race.governor[Governor].name, Playernum, Governor, star_ref.get_name());
    notify_star(g.entity_manager, Playernum, Governor, star, star_msg);
    warn_star(g.entity_manager, Playernum, star, msg);
  } else {
    int gov;
    if (who == Playernum) APcount = 0;
    if (std::isdigit(*argv[2].c_str()) && (gov = std::stoi(argv[2])) >= 0 &&
        gov <= MAXGOVERNORS) {
      push_telegram(who, gov, msg);
      notify(who, gov, notice);
    } else {
      push_telegram_race(g.entity_manager, who, msg);
      notify_race(who, notice);
    }

    auto alien_handle = g.entity_manager.get_race(who);
    if (alien_handle.get()) {
      auto& alien = *alien_handle;
      /* translation modifier increases */
      alien.translate[Playernum - 1] =
          std::min(alien.translate[Playernum - 1] + 2, 100);
    }
  }
  g.out << "Message sent.\n";
  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands
