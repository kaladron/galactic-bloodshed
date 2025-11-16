// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void bless(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  // TODO(jeffbailey): ap_t APcount = 0;
  int amount;
  int Mod;
  char commod;

  if (!g.god) {
    g.out << "You are not privileged to use this command.\n";
    return;
  }
  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Please cs to the planet in question.\n";
    return;
  }
  player_t who = std::stoi(argv[1]);
  if (who < 1 || who > g.entity_manager.num_races()) {
    g.out << "No such player number.\n";
    return;
  }
  if (argv.size() < 3) {
    g.out << "Syntax: bless <player> <what> <+amount>\n";
    return;
  }
  amount = std::stoi(argv[3]);

  auto race_handle = g.entity_manager.get_race(who);
  if (!race_handle.get()) {
    g.out << "Race not found.\n";
    return;
  }
  auto& race = *race_handle;
  /* race characteristics? */
  Mod = 1;

  if (argv[2] == "money") {
    race.governor[0].money += amount;
    warn(who, 0, std::format("Deity gave you {} money.\n", amount));
  } else if (argv[2] == "password") {
    race.password = argv[3];
    warn(who, 0,
         std::format("Deity changed your race password to `{}`\n", argv[3]));
  } else if (argv[2] == "morale") {
    race.morale += amount;
    warn(who, 0, std::format("Deity gave you {} morale.\n", amount));
  } else if (argv[2] == "pods") {
    race.pods = true;
    warn(who, 0, "Deity gave you pod ability.\n");
  } else if (argv[2] == "nopods") {
    race.pods = false;
    warn(who, 0, "Deity took away pod ability.\n");
  } else if (argv[2] == "collectiveiq") {
    race.collective_iq = true;
    warn(who, 0, "Deity gave you collective intelligence.\n");
  } else if (argv[2] == "nocollectiveiq") {
    race.collective_iq = false;
    warn(who, 0, "Deity took away collective intelligence.\n");
  } else if (argv[2] == "maxiq") {
    race.IQ_limit = std::stoi(argv[3]);
    warn(who, 0,
         std::format("Deity gave you a maximum IQ of {}.\n", race.IQ_limit));
  } else if (argv[2] == "mass") {
    race.mass = std::stof(argv[3]);
    warn(who, 0, std::format("Deity gave you {:.2f} mass.\n", race.mass));
  } else if (argv[2] == "metabolism") {
    race.metabolism = std::stof(argv[3]);
    warn(who, 0,
         std::format("Deity gave you {:.2f} metabolism.\n", race.metabolism));
  } else if (argv[2] == "adventurism") {
    race.adventurism = std::stof(argv[3]);
    warn(who, 0,
         std::format("Deity gave you {:<3.0f}% adventurism.\n",
                     race.adventurism * 100.0));
  } else if (argv[2] == "birthrate") {
    race.birthrate = std::stof(argv[3]);
    warn(who, 0,
         std::format("Deity gave you {:.2f} birthrate.\n", race.birthrate));
  } else if (argv[2] == "fertility") {
    race.fertilize = amount;
    warn(who, 0,
         std::format("Deity gave you a fetilization ability of {}.\n", amount));
  } else if (argv[2] == "IQ") {
    race.IQ = amount;
    warn(who, 0, std::format("Deity gave you {} IQ.\n", amount));
  } else if (argv[2] == "fight") {
    race.fighters = amount;
    warn(who, 0,
         std::format("Deity set your fighting ability to {}.\n", amount));
  } else if (argv[2] == "technology") {
    race.tech += (double)amount;
    warn(who, 0, std::format("Deity gave you {} technology.\n", amount));
  } else if (argv[2] == "guest") {
    race.Guest = true;
    warn(who, 0, "Deity turned you into a guest race.\n");
  } else if (argv[2] == "god") {
    race.God = true;
    warn(who, 0, "Deity turned you into a deity race.\n");
  } else if (argv[2] == "mortal") {
    race.God = false;
    race.Guest = false;
    warn(who, 0, "Deity turned you into a mortal race.\n");
    /* sector preferences */
  } else if (argv[2] == "water") {
    race.likes[SectorType::SEC_SEA] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your water preference to {}%\n", amount));
  } else if (argv[2] == "land") {
    race.likes[SectorType::SEC_LAND] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your land preference to {}%\n", amount));
  } else if (argv[2] == "mountain") {
    race.likes[SectorType::SEC_MOUNT] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your mountain preference to {}%\n", amount));
  } else if (argv[2] == "gas") {
    race.likes[SectorType::SEC_GAS] = 0.01 * (double)amount;
    warn(who, 0, std::format("Deity set your gas preference to {}%\n", amount));
  } else if (argv[2] == "ice") {
    race.likes[SectorType::SEC_ICE] = 0.01 * (double)amount;
    warn(who, 0, std::format("Deity set your ice preference to {}%\n", amount));
  } else if (argv[2] == "forest") {
    race.likes[SectorType::SEC_FOREST] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your forest preference to {}%\n", amount));
  } else if (argv[2] == "desert") {
    race.likes[SectorType::SEC_DESERT] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your desert preference to {}%\n", amount));
  } else if (argv[2] == "plated") {
    race.likes[SectorType::SEC_PLATED] = 0.01 * (double)amount;
    warn(who, 0,
         std::format("Deity set your plated preference to {}%\n", amount));
  } else
    Mod = 0;
  if (Mod) return;
  /* ok, must be the planet then */
  commod = argv[2][0];
  auto planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& planet = *planet_handle;
  if (argv[2] == "explorebit") {
    planet.info(who - 1).explored = 1;
    auto star_handle = g.entity_manager.get_star(g.snum);
    if (!star_handle.get()) {
      g.out << "Star not found.\n";
      return;
    }
    auto& star = *star_handle;
    setbit(star.explored(), who);
    warn(who, 0,
         std::format("Deity set your explored bit at /{}/{}.\n", star.get_name(),
                     star.get_planet_name(g.pnum)));
  } else if (argv[2] == "noexplorebit") {
    planet.info(who - 1).explored = 0;
    const auto* star_ptr = g.entity_manager.peek_star(g.snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    warn(who, 0,
         std::format("Deity reset your explored bit at /{}/{}.\n",
                     star_ptr->get_name(), star_ptr->get_planet_name(g.pnum)));
  } else if (argv[2] == "planetpopulation") {
    planet.info(who - 1).popn = std::stoi(argv[3]);
    planet.popn()++;
    const auto* star_ptr = g.entity_manager.peek_star(g.snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    warn(who, 0,
         std::format("Deity set your population variable to {} at /{}/{}.\n",
                     planet.info(who - 1).popn, star_ptr->get_name(),
                     star_ptr->get_planet_name(g.pnum)));
  } else if (argv[2] == "inhabited") {
    auto star_handle = g.entity_manager.get_star(g.snum);
    if (!star_handle.get()) {
      g.out << "Star not found.\n";
      return;
    }
    auto& star = *star_handle;
    setbit(star.inhabited(), Playernum);
    warn(who, 0,
         std::format("Deity has set your inhabited bit for /{}/{}.\n",
                     star.get_name(), star.get_planet_name(g.pnum)));
  } else if (argv[2] == "numsectsowned") {
    planet.info(who - 1).numsectsowned = std::stoi(argv[3]);
    const auto* star_ptr = g.entity_manager.peek_star(g.snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    warn(who, 0,
         std::format(
             "Deity set your \"numsectsowned\" variable at /{}/{} to {}.\n",
             star_ptr->get_name(), star_ptr->get_planet_name(g.pnum),
             planet.info(who - 1).numsectsowned));
  } else {
    const auto* star_ptr = g.entity_manager.peek_star(g.snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    switch (commod) {
      case 'r':
        planet.info(who - 1).resource += amount;
        warn(who, 0,
             std::format("Deity gave you {} resources at {}/{}.\n", amount,
                         star_ptr->get_name(), star_ptr->get_planet_name(g.pnum)));
        break;
      case 'd':
        planet.info(who - 1).destruct += amount;
        warn(who, 0,
             std::format("Deity gave you {} destruct at {}/{}.\n", amount,
                         star_ptr->get_name(), star_ptr->get_planet_name(g.pnum)));
        break;
      case 'f':
        planet.info(who - 1).fuel += amount;
        warn(who, 0,
             std::format("Deity gave you {} fuel at {}/{}.\n", amount,
                         star_ptr->get_name(), star_ptr->get_planet_name(g.pnum)));
        break;
      case 'x':
        planet.info(who - 1).crystals += amount;
        warn(who, 0,
             std::format("Deity gave you {} crystals at {}/{}.\n", amount,
                         star_ptr->get_name(), star_ptr->get_planet_name(g.pnum)));
        break;
      case 'a': {
        auto star_handle = g.entity_manager.get_star(g.snum);
        if (!star_handle.get()) {
          g.out << "Star not found.\n";
          return;
        }
        auto& star = *star_handle;
        star.AP(who - 1) += amount;
        warn(who, 0,
             std::format("Deity gave you {} action points at {}.\n", amount,
                         star.get_name()));
        break;
      }
      default:
        g.out << "No such commodity.\n";
        return;
    }
  }
}
}  // namespace GB::commands
