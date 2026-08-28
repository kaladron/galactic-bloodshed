// SPDX-License-Identifier: Apache-2.0

/// \file declare.cc
/// \brief Declare alliance, neutrality, war, and relations.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import scnlib;
import session;

module commands;

namespace GB::commands {
bool declare(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  player_t n = get_player(g.entity_manager, argv[1]);
  if (n.value == 0) {
    g.out << "No such player.\n";
    return false;
  }

  const auto* alien_peek = g.entity_manager.peek_race(n);
  if (!alien_peek) {
    g.out << "Alien race not found.\n";
    return false;
  }
  const std::string alien_name = alien_peek->name;

  int d_mod = 30;
  std::string news_msg;

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    switch (argv[2][0]) {
      case 'a':
        setbit(race.allied, n);
        clrbit(race.atwar, n);
        if (success(5)) {
          g.out << "But would you want your sister to marry one?\n";
        } else {
          g.out << "Good for you.\n";
        }
        warn_race(
            g.session_registry, g.entity_manager, n,
            std::format(" Player #{} ({}) has declared an alliance with you!\n",
                        Playernum, race.name));
        news_msg = std::format("{} [{}] declares ALLIANCE with {} [{}].\n",
                               race.name, Playernum, alien_name, n);
        d_mod = 30;
        if (argv.size() > 3) {
          auto parsed = scn::scan<int>(argv[3], "{}");
          if (parsed) {
            d_mod = std::max(parsed->value(), 30);
          }
        }
        break;
      case 'n':
        clrbit(race.allied, n);
        clrbit(race.atwar, n);
        g.out << "Done.\n";

        warn_race(
            g.session_registry, g.entity_manager, n,
            std::format(" Player #{} ({}) has declared neutrality with you!\n",
                        Playernum, race.name));
        news_msg = std::format(
            "{} [{}] declares a state of neutrality with {} [{}].\n", race.name,
            Playernum, alien_name, n);
        d_mod = 30;
        break;
      case 'w':
        setbit(race.atwar, n);
        clrbit(race.allied, n);
        if (success(4)) {
          g.out << "Your enemies flaunt their secondary male reproductive "
                   "glands in your\ngeneral direction.\n";
        } else {
          g.out << "Give 'em hell!\n";
        }
        warn_race(
            g.session_registry, g.entity_manager, n,
            std::format(" Player #{} ({}) has declared war against you!\n",
                        Playernum, race.name));
        switch (int_rand(1, 5)) {
          case 1:
            news_msg = std::format("{} [{}] declares WAR on {} [{}].\n",
                                   race.name, Playernum, alien_name, n);
            break;
          case 2:
            news_msg = std::format(
                "{} [{}] has had enough of {} [{}] and declares WAR!\n",
                race.name, Playernum, alien_name, n);
            break;
          case 3:
            news_msg = std::format(
                "{} [{}] decided that it is time to declare WAR on {} [{}]!\n",
                race.name, Playernum, alien_name, n);
            break;
          case 4:
            news_msg = std::format(
                "{} [{}] had no choice but to declare WAR against {} [{}]!\n",
                race.name, Playernum, alien_name, n);
            break;
          case 5:
            news_msg = std::format(
                "{} [{}] says 'screw it!' and declares WAR on {} [{}]!\n",
                race.name, Playernum, alien_name, n);
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
  });

  if (news_msg.empty()) {
    return false;
  }

  g.entity_manager.mutate_race(n, [&](Race& alien) {
    /* They, of course, learn more about you */
    alien.translate[Playernum.value - 1] =
        MIN(alien.translate[Playernum.value - 1] + d_mod, 100);
  });

  post(g.entity_manager, news_msg, NewsType::DECLARATION);
  warn_race(g.session_registry, g.entity_manager, Playernum, news_msg);
  return true;
}

const CommandDescriptor declare_cmd{
    .name = "declare",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::fixed_univ(1),
    .min_args = 3,
    .syntax = "declare <race> <alliance|neutral|war> [<modifier>]",
    .description = "Declare alliance, neutrality, or war with another race",
    .handler = &declare,
};

}  // namespace GB::commands
