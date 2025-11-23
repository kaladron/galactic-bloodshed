// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void declare(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const ap_t APcount = 1;
  player_t n;
  int d_mod;
  std::string news_msg;

  if (Governor) {
    g.out << "Only leaders may declare.\n";
    return;
  }

  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }

  /* look in sdata for APs first */
  /* enufAPs would print something */
  if ((int)Sdata.AP[Playernum - 1] >= APcount) {
    deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    /* otherwise use current star */
  } else if ((g.level == ScopeLevel::LEVEL_STAR ||
              g.level == ScopeLevel::LEVEL_PLAN) &&
             enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1),
                    APcount)) {
    deductAPs(g, APcount, g.snum);
  } else {
    notify(Playernum, Governor,
           std::format("You don't have enough AP's ({})\n", APcount));
    return;
  }

  auto race_handle = g.entity_manager.get_race(Playernum);

  auto alien_handle = g.entity_manager.get_race(n);
  if (!alien_handle.get()) {
    g.out << "Alien race not found.\n";
    return;
  }

  auto& race = *race_handle;
  auto& alien = *alien_handle;

  switch (*argv[2].c_str()) {
    case 'a':
      setbit(race.allied, n);
      clrbit(race.atwar, n);
      if (success(5)) {
        notify(Playernum, Governor,
               "But would you want your sister to marry one?\n");
      } else {
        notify(Playernum, Governor, "Good for you.\n");
      }
      warn_race(
          g.entity_manager, n,
          std::format(" Player #{} ({}) has declared an alliance with you!\n",
                      Playernum, race.name));
      news_msg = std::format("{} [{}] declares ALLIANCE with {} [{}].\n",
                             race.name, Playernum, alien.name, n);
      d_mod = 30;
      if (argv.size() > 3) d_mod = std::stoi(argv[3]);
      d_mod = std::max(d_mod, 30);
      break;
    case 'n':
      clrbit(race.allied, n);
      clrbit(race.atwar, n);
      notify(Playernum, Governor, "Done.\n");

      warn_race(
          g.entity_manager, n,
          std::format(" Player #{} ({}) has declared neutrality with you!\n",
                      Playernum, race.name));
      news_msg =
          std::format("{} [{}] declares a state of neutrality with {} [{}].\n",
                      race.name, Playernum, alien.name, n);
      d_mod = 30;
      break;
    case 'w':
      setbit(race.atwar, n);
      clrbit(race.allied, n);
      if (success(4)) {
        notify(Playernum, Governor,
               "Your enemies flaunt their secondary male reproductive glands "
               "in your\ngeneral direction.\n");
      } else {
        notify(Playernum, Governor, "Give 'em hell!\n");
      }
      warn_race(g.entity_manager, n,
                std::format(" Player #{} ({}) has declared war against you!\n",
                            Playernum, race.name));
      switch (int_rand(1, 5)) {
        case 1:
          news_msg = std::format("{} [{}] declares WAR on {} [{}].\n",
                                 race.name, Playernum, alien.name, n);
          break;
        case 2:
          news_msg = std::format(
              "{} [{}] has had enough of {} [{}] and declares WAR!\n",
              race.name, Playernum, alien.name, n);
          break;
        case 3:
          news_msg = std::format(
              "{} [{}] decided that it is time to declare WAR on {} [{}]!\n",
              race.name, Playernum, alien.name, n);
          break;
        case 4:
          news_msg = std::format(
              "{} [{}] had no choice but to declare WAR against {} [{}]!\n",
              race.name, Playernum, alien.name, n);
          break;
        case 5:
          news_msg = std::format(
              "{} [{}] says 'screw it!' and declares WAR on {} [{}]!\n",
              race.name, Playernum, alien.name, n);
          break;
        default:
          break;
      }
      d_mod = 30;
      break;
    default:
      g.out << "I don't understand.\n";
      return;
  }

  post(news_msg, NewsType::DECLARATION);
  warn_race(g.entity_manager, Playernum, news_msg);

  /* They, of course, learn more about you */
  alien.translate[Playernum - 1] =
      MIN(alien.translate[Playernum - 1] + d_mod, 100);
}
}  // namespace GB::commands
