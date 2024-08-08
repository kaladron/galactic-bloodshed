// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace GB::commands {
void bless(const command_t &argv, GameObj &g) {
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
  if (who < 1 || who > Num_races) {
    g.out << "No such player number.\n";
    return;
  }
  if (argv.size() < 3) {
    g.out << "Syntax: bless <player> <what> <+amount>\n";
    return;
  }
  amount = std::stoi(argv[3]);

  auto &race = races[who - 1];
  /* race characteristics? */
  Mod = 1;

  if (argv[2] == "money") {
    race.governor[0].money += amount;
    sprintf(buf, "Deity gave you %d money.\n", amount);
  } else if (argv[2] == "password") {
    strcpy(race.password, argv[3].c_str());
    sprintf(buf, "Deity changed your race password to `%s'\n", argv[3].c_str());
  } else if (argv[2] == "morale") {
    race.morale += amount;
    sprintf(buf, "Deity gave you %d morale.\n", amount);
  } else if (argv[2] == "pods") {
    race.pods = 1;
    sprintf(buf, "Deity gave you pod ability.\n");
  } else if (argv[2] == "nopods") {
    race.pods = 0;
    sprintf(buf, "Deity took away pod ability.\n");
  } else if (argv[2] == "collectiveiq") {
    race.collective_iq = 1;
    sprintf(buf, "Deity gave you collective intelligence.\n");
  } else if (argv[2] == "nocollectiveiq") {
    race.collective_iq = 0;
    sprintf(buf, "Deity took away collective intelligence.\n");
  } else if (argv[2] == "maxiq") {
    race.IQ_limit = std::stoi(argv[3]);
    sprintf(buf, "Deity gave you a maximum IQ of %d.\n", race.IQ_limit);
  } else if (argv[2] == "mass") {
    race.mass = std::stof(argv[3]);
    sprintf(buf, "Deity gave you %.2f mass.\n", race.mass);
  } else if (argv[2] == "metabolism") {
    race.metabolism = std::stof(argv[3]);
    sprintf(buf, "Deity gave you %.2f metabolism.\n", race.metabolism);
  } else if (argv[2] == "adventurism") {
    race.adventurism = std::stof(argv[3]);
    sprintf(buf, "Deity gave you %-3.0f%% adventurism.\n",
            race.adventurism * 100.0);
  } else if (argv[2] == "birthrate") {
    race.birthrate = std::stof(argv[3]);
    sprintf(buf, "Deity gave you %.2f birthrate.\n", race.birthrate);
  } else if (argv[2] == "fertility") {
    race.fertilize = amount;
    sprintf(buf, "Deity gave you a fetilization ability of %d.\n", amount);
  } else if (argv[2] == "IQ") {
    race.IQ = amount;
    sprintf(buf, "Deity gave you %d IQ.\n", amount);
  } else if (argv[2] == "fight") {
    race.fighters = amount;
    sprintf(buf, "Deity set your fighting ability to %d.\n", amount);
  } else if (argv[2] == "technology") {
    race.tech += (double)amount;
    sprintf(buf, "Deity gave you %d technology.\n", amount);
  } else if (argv[2] == "guest") {
    race.Guest = 1;
    sprintf(buf, "Deity turned you into a guest race.\n");
  } else if (argv[2] == "god") {
    race.God = 1;
    sprintf(buf, "Deity turned you into a deity race.\n");
  } else if (argv[2] == "mortal") {
    race.God = 0;
    race.Guest = 0;
    sprintf(buf, "Deity turned you into a mortal race.\n");
    /* sector preferences */
  } else if (argv[2] == "water") {
    race.likes[SectorType::SEC_SEA] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your water preference to %d%%\n", amount);
  } else if (argv[2] == "land") {
    race.likes[SectorType::SEC_LAND] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your land preference to %d%%\n", amount);
  } else if (argv[2] == "mountain") {
    race.likes[SectorType::SEC_MOUNT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your mountain preference to %d%%\n", amount);
  } else if (argv[2] == "gas") {
    race.likes[SectorType::SEC_GAS] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your gas preference to %d%%\n", amount);
  } else if (argv[2] == "ice") {
    race.likes[SectorType::SEC_ICE] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your ice preference to %d%%\n", amount);
  } else if (argv[2] == "forest") {
    race.likes[SectorType::SEC_FOREST] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your forest preference to %d%%\n", amount);
  } else if (argv[2] == "desert") {
    race.likes[SectorType::SEC_DESERT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your desert preference to %d%%\n", amount);
  } else if (argv[2] == "plated") {
    race.likes[SectorType::SEC_PLATED] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your plated preference to %d%%\n", amount);
  } else
    Mod = 0;
  if (Mod) {
    putrace(race);
    warn(who, 0, buf);
  }
  if (Mod) return;
  /* ok, must be the planet then */
  commod = argv[2][0];
  auto planet = getplanet(g.snum, g.pnum);
  if (argv[2] == "explorebit") {
    planet.info[who - 1].explored = 1;
    stars[g.snum] = getstar(g.snum);
    setbit(stars[g.snum].explored, who);
    putstar(stars[g.snum], g.snum);
    sprintf(buf, "Deity set your explored bit at /%s/%s.\n", stars[g.snum].name,
            stars[g.snum].pnames[g.pnum]);
  } else if (argv[2] == "noexplorebit") {
    planet.info[who - 1].explored = 0;
    sprintf(buf, "Deity reset your explored bit at /%s/%s.\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
  } else if (argv[2] == "planetpopulation") {
    planet.info[who - 1].popn = std::stoi(argv[3]);
    planet.popn++;
    sprintf(buf, "Deity set your population variable to %ld at /%s/%s.\n",
            planet.info[who - 1].popn, stars[g.snum].name,
            stars[g.snum].pnames[g.pnum]);
  } else if (argv[2] == "inhabited") {
    stars[g.snum] = getstar(g.snum);
    setbit(stars[g.snum].inhabited, Playernum);
    putstar(stars[g.snum], g.snum);
    sprintf(buf, "Deity has set your inhabited bit for /%s/%s.\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
  } else if (argv[2] == "numsectsowned") {
    planet.info[who - 1].numsectsowned = std::stoi(argv[3]);
    sprintf(buf, "Deity set your \"numsectsowned\" variable at /%s/%s to %d.\n",
            stars[g.snum].name, stars[g.snum].pnames[g.pnum],
            planet.info[who - 1].numsectsowned);
  } else {
    switch (commod) {
      case 'r':
        planet.info[who - 1].resource += amount;
        sprintf(buf, "Deity gave you %d resources at %s/%s.\n", amount,
                stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
        break;
      case 'd':
        planet.info[who - 1].destruct += amount;
        sprintf(buf, "Deity gave you %d destruct at %s/%s.\n", amount,
                stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
        break;
      case 'f':
        planet.info[who - 1].fuel += amount;
        sprintf(buf, "Deity gave you %d fuel at %s/%s.\n", amount,
                stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
        break;
      case 'x':
        planet.info[who - 1].crystals += amount;
        sprintf(buf, "Deity gave you %d crystals at %s/%s.\n", amount,
                stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
        break;
      case 'a':
        stars[g.snum] = getstar(g.snum);
        stars[g.snum].AP[who - 1] += amount;
        putstar(stars[g.snum], g.snum);
        sprintf(buf, "Deity gave you %d action points at %s.\n", amount,
                stars[g.snum].name);
        break;
      default:
        g.out << "No such commodity.\n";
        return;
    }
  }
  putplanet(planet, stars[g.snum], g.pnum);
  warn_race(who, buf);
}
}  // namespace GB::commands
