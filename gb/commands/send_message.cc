// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/place.h"
#include "gb/tele.h"

module commands;

namespace GB::commands {
void send_message(const command_t &argv, GameObj &g) {
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
  char msg[1000];

  star = 0;  // TODO(jeffbailey): Init to zero.
  who = 0;   // TODO(jeffbailey): Init to zero.

  to_star = to_block = 0;

  if (argv.size() < 2) {
    g.out << "Send what?\n";
    return;
  }
  if (postit) {
    auto &race = races[Playernum - 1];
    sprintf(msg, "%s \"%s\" [%d,%d]: ", race.name, race.governor[Governor].name,
            Playernum, Governor);
    /* put the message together */
    for (j = 1; j < argv.size(); j++) {
      sprintf(buf, "%s ", argv[j].c_str());
      strcat(msg, buf);
    }
    strcat(msg, "\n");
    post(msg, NewsType::ANNOUNCE);
    return;
  }
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Sending message to alliance block.\n";
    if (!(who = get_player(argv[2]))) {
      g.out << "No such alliance block.\n";
      return;
    }
    auto &alien = races[who - 1];
    APcount *= !alien.God;
  } else if (argv[1] == "star") {
    to_star = 1;
    g.out << "Sending message to star system.\n";
    Place where{g, argv[2], true};
    if (where.err || where.level != ScopeLevel::LEVEL_STAR) {
      g.out << "No such star.\n";
      return;
    }
    star = where.snum;
    stars[star] = getstar(star);
  } else {
    if (!(who = get_player(argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
    auto &alien = races[who - 1];
    APcount *= !alien.God;
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't send messages from universal scope.\n";
      return;

    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can't send messages from ship scope.\n";
      return;

    default:
      stars[g.snum] = getstar(g.snum);
      if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1],
                  APcount))
        return;
      break;
  }

  auto &race = races[Playernum - 1];

  /* send the message */
  if (to_block)
    sprintf(msg, "%s \"%s\" [%d,%d] to %s [%d]: ", race.name,
            race.governor[Governor].name, Playernum, Governor,
            Blocks[who - 1].name, who);
  else if (to_star)
    sprintf(msg, "%s \"%s\" [%d,%d] to inhabitants of %s: ", race.name,
            race.governor[Governor].name, Playernum, Governor,
            stars[star].name);
  else
    sprintf(msg, "%s \"%s\" [%d,%d]: ", race.name, race.governor[Governor].name,
            Playernum, Governor);

  if (to_star || to_block || isdigit(*argv[2].c_str()))
    start = 3;
  else if (postit)
    start = 1;
  else
    start = 2;
  /* put the message together */
  for (j = start; j < argv.size(); j++) {
    sprintf(buf, "%s ", argv[j].c_str());
    strcat(msg, buf);
  }
  /* post it */
  sprintf(buf,
          "%s \"%s\" [%d,%d] has sent you a telegram. Use `read' to read it.\n",
          race.name, race.governor[Governor].name, Playernum, Governor);
  if (to_block) {
    uint64_t dummy = (Blocks[who - 1].invite & Blocks[who - 1].pledge);
    sprintf(buf,
            "%s \"%s\" [%d,%d] sends a message to %s [%d] alliance block.\n",
            race.name, race.governor[Governor].name, Playernum, Governor,
            Blocks[who - 1].name, who);
    for (i = 1; i <= Num_races; i++) {
      if (isset(dummy, i)) {
        notify_race(i, buf);
        push_telegram_race(i, msg);
      }
    }
  } else if (to_star) {
    sprintf(buf, "%s \"%s\" [%d,%d] sends a stargram to %s.\n", race.name,
            race.governor[Governor].name, Playernum, Governor,
            stars[star].name);
    notify_star(Playernum, Governor, star, buf);
    warn_star(Playernum, star, msg);
  } else {
    int gov;
    if (who == Playernum) APcount = 0;
    if (isdigit(*argv[2].c_str()) && (gov = std::stoi(argv[2])) >= 0 &&
        gov <= MAXGOVERNORS) {
      push_telegram(who, gov, msg);
      notify(who, gov, buf);
    } else {
      push_telegram_race(who, msg);
      notify_race(who, buf);
    }

    auto &alien = races[who - 1];
    /* translation modifier increases */
    alien.translate[Playernum - 1] =
        std::min(alien.translate[Playernum - 1] + 2, 100);
    putrace(alien);
  }
  g.out << "Message sent.\n";
  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands
