// SPDX-License-Identifier: Apache-2.0

/// \file bless.cc
/// \brief Bestow divine blessings upon a player.

module;

import session;
import gb.entities;
import gb.services;
import notification;
import std;
#undef stdout

module commands;

namespace GB::commands {

bool bless(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  int amount;
  int Mod;
  char commod;

  player_t who = std::stoi(argv[1]);
  if (who < 1 || who > g.entity_manager.num_races()) {
    g.out << "No such player number.\n";
    return false;
  }
  amount = std::stoi(argv[3]);

  /* race characteristics? */
  Mod = 1;

  if (argv[2] == "money") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.governor[0].money += amount;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} money.\n", amount));
    });
  } else if (argv[2] == "password") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.password = argv[3];
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity changed your race password to `{}`\n", argv[3]));
    });
  } else if (argv[2] == "morale") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.morale += amount;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} morale.\n", amount));
    });
  } else if (argv[2] == "pods") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.pods = true;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity gave you pod ability.\n");
    });
  } else if (argv[2] == "nopods") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.pods = false;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity took away pod ability.\n");
    });
  } else if (argv[2] == "collectiveiq") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.collective_iq = true;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity gave you collective intelligence.\n");
    });
  } else if (argv[2] == "nocollectiveiq") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.collective_iq = false;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity took away collective intelligence.\n");
    });
  } else if (argv[2] == "maxiq") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.IQ_limit = std::stoi(argv[3]);
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity gave you a maximum IQ of {}.\n", race.IQ_limit));
    });
  } else if (argv[2] == "mass") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.mass = std::stof(argv[3]);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {:.2f} mass.\n", race.mass));
    });
  } else if (argv[2] == "metabolism") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.metabolism = std::stof(argv[3]);
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity gave you {:.2f} metabolism.\n", race.metabolism));
    });
  } else if (argv[2] == "adventurism") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.adventurism = std::stof(argv[3]);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {:<3.0f}% adventurism.\n",
                              race.adventurism * 100.0));
    });
  } else if (argv[2] == "birthrate") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.birthrate = std::stof(argv[3]);
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity gave you {:.2f} birthrate.\n", race.birthrate));
    });
  } else if (argv[2] == "fertility") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.fertilize = amount;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you a fetilization ability of {}.\n",
                              amount));
    });
  } else if (argv[2] == "IQ") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.IQ = amount;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} IQ.\n", amount));
    });
  } else if (argv[2] == "fight") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.fighters = amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your fighting ability to {}.\n", amount));
    });
  } else if (argv[2] == "technology") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.tech += (double)amount;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} technology.\n", amount));
    });
  } else if (argv[2] == "guest") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.Guest = true;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity turned you into a guest race.\n");
    });
  } else if (argv[2] == "god") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.God = true;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity turned you into a deity race.\n");
    });
  } else if (argv[2] == "mortal") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.God = false;
      race.Guest = false;
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  "Deity turned you into a mortal race.\n");
    });
    /* sector preferences */
  } else if (argv[2] == "water") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_SEA] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your water preference to {}%\n", amount));
    });
  } else if (argv[2] == "land") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_LAND] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your land preference to {}%\n", amount));
    });
  } else if (argv[2] == "mountain") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_MOUNT] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your mountain preference to {}%\n", amount));
    });
  } else if (argv[2] == "gas") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_GAS] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your gas preference to {}%\n", amount));
    });
  } else if (argv[2] == "ice") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_ICE] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your ice preference to {}%\n", amount));
    });
  } else if (argv[2] == "forest") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_FOREST] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your forest preference to {}%\n", amount));
    });
  } else if (argv[2] == "desert") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_DESERT] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your desert preference to {}%\n", amount));
    });
  } else if (argv[2] == "plated") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.likes[SectorType::SEC_PLATED] = 0.01 * (double)amount;
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity set your plated preference to {}%\n", amount));
    });
  } else
    Mod = 0;
  if (Mod) return true;
  /* ok, must be the planet then */
  commod = argv[2][0];
  if (argv[2] == "explorebit") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).explored = 1;
    });
    g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
      setbit(star.explored(), who);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity set your explored bit at /{}/{}.\n",
                              star.get_name(), star.get_planet_name(g.pnum())));
    });
  } else if (argv[2] == "noexplorebit") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).explored = 0;
    });
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(g.session_registry, g.entity_manager, who, 0,
                std::format("Deity reset your explored bit at /{}/{}.\n",
                            star.get_name(), star.get_planet_name(g.pnum())));
  } else if (argv[2] == "planetpopulation") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).popn = std::stoi(argv[3]);
      planet.popn()++;
    });
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(
        g.session_registry, g.entity_manager, who, 0,
        std::format("Deity set your population variable to {} at /{}/{}.\n",
                    std::stoi(argv[3]), star.get_name(),
                    star.get_planet_name(g.pnum())));
  } else if (argv[2] == "inhabited") {
    g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
      setbit(star.inhabited(), Playernum);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity has set your inhabited bit for /{}/{}.\n",
                              star.get_name(), star.get_planet_name(g.pnum())));
    });
  } else if (argv[2] == "numsectsowned") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).numsectsowned = std::stoi(argv[3]);
    });
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(
        g.session_registry, g.entity_manager, who, 0,
        std::format(
            "Deity set your \"numsectsowned\" variable at /{}/{} to {}.\n",
            star.get_name(), star.get_planet_name(g.pnum()),
            std::stoi(argv[3])));
  } else {
    const auto& star = *g.entity_manager.peek_star(g.snum());
    switch (commod) {
      case 'r':
        g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
          planet.info(who).resource += amount;
        });
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity gave you {} resources at {}/{}.\n",
                                amount, star.get_name(),
                                star.get_planet_name(g.pnum())));
        break;
      case 'd':
        g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
          planet.info(who).destruct += amount;
        });
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity gave you {} destruct at {}/{}.\n",
                                amount, star.get_name(),
                                star.get_planet_name(g.pnum())));
        break;
      case 'f':
        g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
          planet.info(who).fuel += amount;
        });
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity gave you {} fuel at {}/{}.\n", amount,
                                star.get_name(),
                                star.get_planet_name(g.pnum())));
        break;
      case 'x':
        g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
          planet.info(who).crystals += amount;
        });
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity gave you {} crystals at {}/{}.\n",
                                amount, star.get_name(),
                                star.get_planet_name(g.pnum())));
        break;
      case 'a': {
        g.entity_manager.mutate_star(g.snum(),
                                     [&](Star& s) { s.AP(who) += amount; });
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity gave you {} action points at {}.\n",
                                amount, star.get_name()));
        break;
      }
      default:
        g.out << "No such commodity.\n";
        return false;
    }
  }
  return true;
}

const CommandDescriptor bless_cmd{
    .name = "bless",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 4,
    .syntax = "bless <player> <what> <+amount>",
    .description = "Bestow divine blessings upon a player (deity only)",
    .handler = &bless,
};

}  // namespace GB::commands
