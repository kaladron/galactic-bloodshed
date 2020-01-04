// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* name.c -- rename something to something else */

import gblib;
import std;

#include "gb/name.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/dissolve.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/star.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

namespace {
int revolt(Planet &pl, const player_t victim, const player_t agent) {
  int revolted_sectors = 0;

  auto smap = getsmap(pl);
  for (auto &s : smap) {
    if (s.owner != victim || s.popn == 0) continue;

    // Revolt rate is a function of tax rate.
    if (!success(pl.info[victim - 1].tax)) continue;

    if (static_cast<unsigned long>(long_rand(1, s.popn)) <=
        10 * races[victim - 1]->fighters * s.troops)
      continue;

    // Revolt successful.
    s.owner = agent;                   /* enemy gets it */
    s.popn = int_rand(1, (int)s.popn); /* some people killed */
    s.troops = 0;                      /* all troops destroyed */
    pl.info[victim - 1].numsectsowned -= 1;
    pl.info[agent - 1].numsectsowned += 1;
    pl.info[victim - 1].mob_points -= s.mobilization;
    pl.info[agent - 1].mob_points += s.mobilization;
    revolted_sectors++;
  }
  putsmap(smap, pl);

  return revolted_sectors;
}
}

void personal(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  ss_message << std::ends;
  std::string message = ss_message.str();

  if (g.governor != 0) {
    g.out << "Only the leader can do this.\n";
    return;
  }
  auto Race = races[Playernum - 1];
  strncpy(Race->info, message.c_str(), PERSONALSIZE - 1);
  putrace(Race);
}

void bless(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  // TODO(jeffbailey): int APcount = 0;
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

  racetype *Race = races[who - 1];
  /* race characteristics? */
  Mod = 1;

  if (argv[2] == "money") {
    Race->governor[0].money += amount;
    sprintf(buf, "Deity gave you %d money.\n", amount);
  } else if (argv[2] == "password") {
    strcpy(Race->password, argv[3].c_str());
    sprintf(buf, "Deity changed your race password to `%s'\n", argv[3].c_str());
  } else if (argv[2] == "morale") {
    Race->morale += amount;
    sprintf(buf, "Deity gave you %d morale.\n", amount);
  } else if (argv[2] == "pods") {
    Race->pods = 1;
    sprintf(buf, "Deity gave you pod ability.\n");
  } else if (argv[2] == "nopods") {
    Race->pods = 0;
    sprintf(buf, "Deity took away pod ability.\n");
  } else if (argv[2] == "collectiveiq") {
    Race->collective_iq = 1;
    sprintf(buf, "Deity gave you collective intelligence.\n");
  } else if (argv[2] == "nocollectiveiq") {
    Race->collective_iq = 0;
    sprintf(buf, "Deity took away collective intelligence.\n");
  } else if (argv[2] == "maxiq") {
    Race->IQ_limit = std::stoi(argv[3]);
    sprintf(buf, "Deity gave you a maximum IQ of %d.\n", Race->IQ_limit);
  } else if (argv[2] == "mass") {
    Race->mass = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f mass.\n", Race->mass);
  } else if (argv[2] == "metabolism") {
    Race->metabolism = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f metabolism.\n", Race->metabolism);
  } else if (argv[2] == "adventurism") {
    Race->adventurism = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %-3.0f%% adventurism.\n",
            Race->adventurism * 100.0);
  } else if (argv[2] == "birthrate") {
    Race->birthrate = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f birthrate.\n", Race->birthrate);
  } else if (argv[2] == "fertility") {
    Race->fertilize = amount;
    sprintf(buf, "Deity gave you a fetilization ability of %d.\n", amount);
  } else if (argv[2] == "IQ") {
    Race->IQ = amount;
    sprintf(buf, "Deity gave you %d IQ.\n", amount);
  } else if (argv[2] == "fight") {
    Race->fighters = amount;
    sprintf(buf, "Deity set your fighting ability to %d.\n", amount);
  } else if (argv[2] == "technology") {
    Race->tech += (double)amount;
    sprintf(buf, "Deity gave you %d technology.\n", amount);
  } else if (argv[2] == "guest") {
    Race->Guest = 1;
    sprintf(buf, "Deity turned you into a guest race.\n");
  } else if (argv[2] == "god") {
    Race->God = 1;
    sprintf(buf, "Deity turned you into a deity race.\n");
  } else if (argv[2] == "mortal") {
    Race->God = 0;
    Race->Guest = 0;
    sprintf(buf, "Deity turned you into a mortal race.\n");
    /* sector preferences */
  } else if (argv[2] == "water") {
    Race->likes[SectorType::SEC_SEA] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your water preference to %d%%\n", amount);
  } else if (argv[2] == "land") {
    Race->likes[SectorType::SEC_LAND] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your land preference to %d%%\n", amount);
  } else if (argv[2] == "mountain") {
    Race->likes[SectorType::SEC_MOUNT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your mountain preference to %d%%\n", amount);
  } else if (argv[2] == "gas") {
    Race->likes[SectorType::SEC_GAS] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your gas preference to %d%%\n", amount);
  } else if (argv[2] == "ice") {
    Race->likes[SectorType::SEC_ICE] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your ice preference to %d%%\n", amount);
  } else if (argv[2] == "forest") {
    Race->likes[SectorType::SEC_FOREST] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your forest preference to %d%%\n", amount);
  } else if (argv[2] == "desert") {
    Race->likes[SectorType::SEC_DESERT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your desert preference to %d%%\n", amount);
  } else if (argv[2] == "plated") {
    Race->likes[SectorType::SEC_PLATED] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your plated preference to %d%%\n", amount);
  } else
    Mod = 0;
  if (Mod) {
    putrace(Race);
    warn(who, 0, buf);
  }
  if (Mod) return;
  /* ok, must be the planet then */
  commod = argv[2][0];
  auto planet = getplanet(g.snum, g.pnum);
  if (argv[2] == "explorebit") {
    planet.info[who - 1].explored = 1;
    getstar(&Stars[g.snum], g.snum);
    setbit(Stars[g.snum]->explored, who);
    putstar(Stars[g.snum], g.snum);
    sprintf(buf, "Deity set your explored bit at /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (argv[2] == "noexplorebit") {
    planet.info[who - 1].explored = 0;
    sprintf(buf, "Deity reset your explored bit at /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (argv[2] == "planetpopulation") {
    planet.info[who - 1].popn = std::stoi(argv[3]);
    planet.popn++;
    sprintf(buf, "Deity set your population variable to %ld at /%s/%s.\n",
            planet.info[who - 1].popn, Stars[g.snum]->name,
            Stars[g.snum]->pnames[g.pnum]);
  } else if (argv[2] == "inhabited") {
    getstar(&Stars[g.snum], g.snum);
    setbit(Stars[g.snum]->inhabited, Playernum);
    putstar(Stars[g.snum], g.snum);
    sprintf(buf, "Deity has set your inhabited bit for /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (argv[2] == "numsectsowned") {
    planet.info[who - 1].numsectsowned = std::stoi(argv[3]);
    sprintf(buf, "Deity set your \"numsectsowned\" variable at /%s/%s to %d.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum],
            planet.info[who - 1].numsectsowned);
  } else {
    switch (commod) {
      case 'r':
        planet.info[who - 1].resource += amount;
        sprintf(buf, "Deity gave you %d resources at %s/%s.\n", amount,
                Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
        break;
      case 'd':
        planet.info[who - 1].destruct += amount;
        sprintf(buf, "Deity gave you %d destruct at %s/%s.\n", amount,
                Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
        break;
      case 'f':
        planet.info[who - 1].fuel += amount;
        sprintf(buf, "Deity gave you %d fuel at %s/%s.\n", amount,
                Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
        break;
      case 'x':
        planet.info[who - 1].crystals += amount;
        sprintf(buf, "Deity gave you %d crystals at %s/%s.\n", amount,
                Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
        break;
      case 'a':
        getstar(&Stars[g.snum], g.snum);
        Stars[g.snum]->AP[who - 1] += amount;
        putstar(Stars[g.snum], g.snum);
        sprintf(buf, "Deity gave you %d action points at %s.\n", amount,
                Stars[g.snum]->name);
        break;
      default:
        g.out << "No such commodity.\n";
        return;
    }
  }
  putplanet(planet, Stars[g.snum], g.pnum);
  warn_race(who, buf);
}

void insurgency(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 10;
  int who;
  int eligible;
  int them = 0;
  racetype *Race;
  racetype *alien;
  double x;
  int changed_hands;
  int chance;
  int i;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must 'cs' to the planet you wish to try it on.\n";
    return;
  }
  if (!control(*Stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  /*  if(argv.size()<3) {
        notify(Playernum, Governor, "The correct syntax is 'insurgency <race>
    <money>'\n");
        return;
    }*/
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount))
    return;
  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  Race = races[Playernum - 1];
  alien = races[who - 1];
  if (alien->Guest) {
    g.out << "Don't be such a dickweed.\n";
    return;
  }
  if (who == Playernum) {
    g.out << "You can't revolt against yourself!\n";
    return;
  }
  eligible = 0;
  them = 0;
  for (i = 0; i < Stars[g.snum]->numplanets; i++) {
    auto p = getplanet(g.snum, i);
    eligible += p.info[Playernum - 1].popn;
    them += p.info[who - 1].popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency\n.";
    return;
  }
  auto p = getplanet(g.snum, g.pnum);

  if (!p.info[who - 1].popn) {
    g.out << "This player does not occupy this planet.\n";
    return;
  }

  int amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to use a positive amount of money.\n";
    return;
  }
  if (Race->governor[Governor].money < amount) {
    g.out << "Nice try.\n";
    return;
  }

  x = INSURG_FACTOR * (double)amount * (double)p.info[who - 1].tax /
      (double)p.info[who - 1].popn;
  x *= morale_factor((double)(Race->morale - alien->morale));
  x *= morale_factor((double)(eligible - them) / 50.0);
  x *= morale_factor(10.0 *
                     (double)(Race->fighters * p.info[Playernum - 1].troops -
                              alien->fighters * p.info[who - 1].troops)) /
       50.0;
  sprintf(buf, "x = %f\n", x);
  notify(Playernum, Governor, buf);
  chance = round_rand(200.0 * atan((double)x) / 3.14159265);
  sprintf(long_buf, "%s/%s: %s [%d] tries insurgency vs %s [%d]\n",
          Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum], Race->name,
          Playernum, alien->name, who);
  sprintf(buf, "\t%s: %d total civs [%d]  opposing %d total civs [%d]\n",
          Stars[g.snum]->name, eligible, Playernum, them, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %ld morale [%d] vs %ld morale [%d]\n", Race->morale,
          Playernum, alien->morale, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %d money against %ld population at tax rate %d%%\n",
          amount, p.info[who - 1].popn, p.info[who - 1].tax);
  strcat(long_buf, buf);
  sprintf(buf, "Success chance is %d%%\n", chance);
  strcat(long_buf, buf);
  if (success(chance)) {
    changed_hands = revolt(p, who, Playernum);
    notify(Playernum, Governor, long_buf);
    sprintf(buf, "Success!  You liberate %d sector%s.\n", changed_hands,
            (changed_hands == 1) ? "" : "s");
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "A revolt on /%s/%s instigated by %s [%d] costs you %d sector%s\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum], Race->name,
            Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
    strcat(long_buf, buf);
    warn(who, Stars[g.snum]->governor[who - 1], long_buf);
    p.info[Playernum - 1].tax = p.info[who - 1].tax;
    /* you inherit their tax rate (insurgency wars he he ) */
    sprintf(buf, "/%s/%s: Successful insurgency by %s [%d] against %s [%d]\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum], Race->name,
            Playernum, alien->name, who);
    post(buf, DECLARATION);
  } else {
    notify(Playernum, Governor, long_buf);
    g.out << "The insurgency failed!\n";
    sprintf(buf, "A revolt on /%s/%s instigated by %s [%d] fails\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum], Race->name,
            Playernum);
    strcat(long_buf, buf);
    warn(who, Stars[g.snum]->governor[who - 1], long_buf);
    sprintf(buf, "/%s/%s: Failed insurgency by %s [%d] against %s [%d]\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum], Race->name,
            Playernum, alien->name, who);
    post(buf, DECLARATION);
  }
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
  Race->governor[Governor].money -= amount;
  putrace(Race);
}

void pay(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  int who;
  int amount;
  racetype *Race;
  racetype *alien;

  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor) {
    g.out << "You are not authorized to do that.\n";
    return;
  }
  Race = races[Playernum - 1];
  alien = races[who - 1];

  amount = std::stoi(argv[2]);
  if (amount < 0) {
    g.out << "You have to give a player a positive amount of money.\n";
    return;
  }
  if (Race->Guest) {
    g.out << "Nice try. Your attempt has been duly noted.\n";
    return;
  }
  if (Race->governor[Governor].money < amount) {
    g.out << "You don't have that much money to give!\n";
    return;
  }

  Race->governor[Governor].money -= amount;
  alien->governor[0].money += amount;
  sprintf(buf, "%s [%d] payed you %d.\n", Race->name, Playernum, amount);
  warn(who, 0, buf);
  sprintf(buf, "%d payed to %s [%d].\n", amount, alien->name, who);
  notify(Playernum, Governor, buf);

  sprintf(buf, "%s [%d] pays %s [%d].\n", Race->name, Playernum, alien->name,
          who);
  post(buf, TRANSFER);

  putrace(alien);
  putrace(Race);
}

void give(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 5;
  int who;
  racetype *Race;
  racetype *alien;

  if (!(who = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }
  if (Governor) {
    g.out << "You are not authorized to do that.\n";
    return;
  }
  alien = races[who - 1];
  Race = races[Playernum - 1];
  if (alien->Guest && !Race->God) {
    g.out << "You can't give this player anything.\n";
    return;
  }
  if (Race->Guest) {
    g.out << "You can't give anyone anything.\n";
    return;
  }
  /* check to see if both players are mutually allied */
  if (!Race->God &&
      !(isset(Race->allied, who) && isset(alien->allied, Playernum))) {
    g.out << "You two are not mutually allied.\n";
    return;
  }
  auto shipno = string_to_shipnum(argv[2]);
  if (!shipno) {
    g.out << "Illegal ship number.\n";
    return;
  }

  auto ship = getship(*shipno);
  if (!ship) {
    g.out << "No such ship.\n";
    return;
  }

  if (ship->owner != Playernum || !ship->alive) {
    DontOwnErr(Playernum, Governor, *shipno);
    return;
  }
  if (ship->type == ShipType::STYPE_POD) {
    g.out << "You cannot change the ownership of spore pods.\n";
    return;
  }

  if ((ship->popn + ship->troops) && !Race->God) {
    g.out << "You can't give this ship away while it has crew/mil on board.\n";
    return;
  }
  if (ship->ships && !Race->God) {
    g.out
        << "You can't give away this ship, it has other ships loaded on it.\n";
    return;
  }
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        return;
      }
      break;
    default:
      if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1],
                  APcount)) {
        return;
      }
      break;
  }

  ship->owner = who;
  ship->governor = 0; /* give to the leader */
  capture_stuff(*ship, g);

  putship(&*ship);

  /* set inhabited/explored bits */
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      break;
    case ScopeLevel::LEVEL_STAR:
      getstar(&(Stars[ship->storbits]), (int)ship->storbits);
      setbit(Stars[ship->storbits]->explored, who);
      putstar(Stars[ship->storbits], (int)ship->storbits);
      break;
    case ScopeLevel::LEVEL_PLAN: {
      getstar(&(Stars[ship->storbits]), (int)ship->storbits);
      setbit(Stars[ship->storbits]->explored, who);
      putstar(Stars[ship->storbits], (int)ship->storbits);

      auto planet = getplanet((int)ship->storbits, (int)ship->pnumorbits);
      planet.info[who - 1].explored = 1;
      putplanet(planet, Stars[ship->storbits], (int)ship->pnumorbits);

    } break;
    default:
      g.out << "Something wrong with this ship's scope.\n";
      return;
  }

  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      deductAPs(Playernum, Governor, APcount, 0, 1);
      return;
    default:
      deductAPs(Playernum, Governor, APcount, g.snum, 0);
      break;
  }
  g.out << "Owner changed.\n";
  sprintf(buf, "%s [%d] gave you %s at %s.\n", Race->name, Playernum,
          ship_to_string(*ship).c_str(), prin_ship_orbits(&*ship));
  warn(who, 0, buf);

  if (!Race->God) {
    sprintf(buf, "%s [%d] gives %s [%d] a ship.\n", Race->name, Playernum,
            alien->name, who);
    post(buf, TRANSFER);
  }
}

void page(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = g.god ? 0 : 1;
  int i;
  int who;
  int gov;
  int to_block;
  racetype *Race;
  racetype *alien;

  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount))
    return;

  gov = 0;  // TODO(jeffbailey): Init to zero.
  to_block = 0;
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Paging alliance block.\n";
    who = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
    gov = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
  } else {
    if (!(who = get_player(argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
    if (argv.size() > 1) gov = std::stoi(argv[2]);
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "You can't make pages at universal scope.\n";
      break;
    default:
      getstar(&Stars[g.snum], g.snum);
      if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1],
                  APcount)) {
        return;
      }

      Race = races[Playernum - 1];

      sprintf(buf, "%s \"%s\" page(s) you from the %s star system.\n",
              Race->name, Race->governor[Governor].name, Stars[g.snum]->name);

      if (to_block) {
        uint64_t dummy =
            Blocks[Playernum - 1].invite & Blocks[Playernum - 1].pledge;
        for (i = 1; i <= Num_races; i++)
          if (isset(dummy, i) && i != Playernum) notify_race(i, buf);
      } else {
        if (argv.size() > 1)
          notify(who, gov, buf);
        else
          notify_race(who, buf);
      }

      g.out << "Request sent.\n";
      break;
  }
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
}

void send_message(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  bool postit = argv[0] == "post";
  int APcount;
  if (postit) {
    APcount = 0;
  } else {
    APcount = g.god ? 0 : 1;
  }
  int who;
  int i;
  int j;
  int to_block;
  int to_star;
  int star;
  int start;
  char msg[1000];
  racetype *Race;
  racetype *alien;

  star = 0;  // TODO(jeffbailey): Init to zero.
  who = 0;   // TODO(jeffbailey): Init to zero.

  to_star = to_block = 0;

  if (argv.size() < 2) {
    g.out << "Send what?\n";
    return;
  }
  if (postit) {
    Race = races[Playernum - 1];
    sprintf(msg, "%s \"%s\" [%d,%d]: ", Race->name,
            Race->governor[Governor].name, Playernum, Governor);
    /* put the message together */
    for (j = 1; j < argv.size(); j++) {
      sprintf(buf, "%s ", argv[j].c_str());
      strcat(msg, buf);
    }
    strcat(msg, "\n");
    post(msg, ANNOUNCE);
    return;
  }
  if (argv[1] == "block") {
    to_block = 1;
    g.out << "Sending message to alliance block.\n";
    if (!(who = get_player(argv[2]))) {
      g.out << "No such alliance block.\n";
      return;
    }
    alien = races[who - 1];
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
    getstar(&(Stars[star]), star);
  } else {
    if (!(who = get_player(argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
    alien = races[who - 1];
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
      getstar(&Stars[g.snum], g.snum);
      if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1],
                  APcount))
        return;
      break;
  }

  Race = races[Playernum - 1];

  /* send the message */
  if (to_block)
    sprintf(msg, "%s \"%s\" [%d,%d] to %s [%d]: ", Race->name,
            Race->governor[Governor].name, Playernum, Governor,
            Blocks[who - 1].name, who);
  else if (to_star)
    sprintf(msg, "%s \"%s\" [%d,%d] to inhabitants of %s: ", Race->name,
            Race->governor[Governor].name, Playernum, Governor,
            Stars[star]->name);
  else
    sprintf(msg, "%s \"%s\" [%d,%d]: ", Race->name,
            Race->governor[Governor].name, Playernum, Governor);

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
          Race->name, Race->governor[Governor].name, Playernum, Governor);
  if (to_block) {
    uint64_t dummy = (Blocks[who - 1].invite & Blocks[who - 1].pledge);
    sprintf(buf,
            "%s \"%s\" [%d,%d] sends a message to %s [%d] alliance block.\n",
            Race->name, Race->governor[Governor].name, Playernum, Governor,
            Blocks[who - 1].name, who);
    for (i = 1; i <= Num_races; i++) {
      if (isset(dummy, i)) {
        notify_race(i, buf);
        push_telegram_race(i, msg);
      }
    }
  } else if (to_star) {
    sprintf(buf, "%s \"%s\" [%d,%d] sends a stargram to %s.\n", Race->name,
            Race->governor[Governor].name, Playernum, Governor,
            Stars[star]->name);
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

    alien = races[who - 1];
    /* translation modifier increases */
    alien->translate[Playernum - 1] =
        std::min(alien->translate[Playernum - 1] + 2, 100);
    putrace(alien);
  }
  g.out << "Message sent.\n";
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
}

void read_messages(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (argv.size() == 1 || argv[1] == "telegram")
    teleg_read(g);
  else if (argv[1] == "news") {
    notify(Playernum, Governor, CUTE_MESSAGE);
    notify(Playernum, Governor,
           "\n----------        Declarations        ----------\n");
    news_read(Playernum, Governor, DECLARATION);
    notify(Playernum, Governor,
           "\n----------           Combat           ----------\n");
    news_read(Playernum, Governor, COMBAT);
    notify(Playernum, Governor,
           "\n----------          Business          ----------\n");
    news_read(Playernum, Governor, TRANSFER);
    notify(Playernum, Governor,
           "\n----------          Bulletins         ----------\n");
    news_read(Playernum, Governor, ANNOUNCE);
  } else
    g.out << "Read what?\n";
}

void motto(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  ss_message << std::ends;
  std::string message = ss_message.str();

  if (Governor) {
    g.out << "You are not authorized to do this.\n";
    return;
  }
  strncpy(Blocks[Playernum - 1].motto, message.c_str(), MOTTOSIZE - 1);
  Putblock(Blocks);
  g.out << "Done.\n";
}

void name(const command_t &argv, GameObj &g) {
  int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  char *ch;
  int spaces;
  unsigned char check = 0;
  char string[1024];
  char tmp[128];
  racetype *Race;

  if (argv.size() < 3 || !isalnum(argv[2][0])) {
    g.out << "Illegal name format.\n";
    return;
  }

  sprintf(buf, "%s", argv[2].c_str());
  for (int i = 3; i < argv.size(); i++) {
    sprintf(tmp, " %s", argv[i].c_str());
    strcat(buf, tmp);
  }

  sprintf(string, "%s", buf);

  /* make sure there are no ^'s or '/' in name,
    also make sure the name has at least 1 character in it */
  ch = string;
  spaces = 0;
  while (*ch != '\0') {
    check |=
        ((!isalnum(*ch) && !(*ch == ' ') && !(*ch == '.')) || (*ch == '/'));
    ch++;
    if (*ch == ' ') spaces++;
  }

  if (spaces == strlen(buf)) {
    g.out << "Illegal name.\n";
    return;
  }

  if (strlen(buf) < 1 || check) {
    sprintf(buf, "Illegal name %s.\n", check ? "form" : "length");
    notify(Playernum, Governor, buf);
    return;
  }

  if (argv[1] == "ship") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(g.shipno);
      strncpy(ship->name, buf, SHIP_NAMESIZE);
      putship(&*ship);
      g.out << "Name set.\n";
      return;
    }
    g.out << "You have to 'cs' to a ship to name it.\n";
    return;
  }
  if (argv[1] == "class") {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      auto ship = getship(g.shipno);
      if (ship->type != ShipType::OTYPE_FACTORY) {
        g.out << "You are not at a factory!\n";
        return;
      }
      if (ship->on) {
        g.out << "This factory is already on line.\n";
        return;
      }
      strncpy(ship->shipclass, buf, SHIP_NAMESIZE - 1);
      putship(&*ship);
      g.out << "Class set.\n";
      return;
    }
    g.out << "You have to 'cs' to a factory to name the ship class.\n";
    return;
  }
  if (argv[1] == "block") {
    /* name your alliance block */
    if (Governor) {
      g.out << "You are not authorized to do this.\n";
      return;
    }
    strncpy(Blocks[Playernum - 1].name, buf, RNAMESIZE - 1);
    Putblock(Blocks);
    g.out << "Done.\n";
  } else if (argv[1] == "star") {
    if (g.level == ScopeLevel::LEVEL_STAR) {
      Race = races[Playernum - 1];
      if (!Race->God) {
        g.out << "Only dieties may name a star.\n";
        return;
      }
      strncpy(Stars[g.snum]->name, buf, NAMESIZE - 1);
      putstar(Stars[g.snum], g.snum);
    } else {
      g.out << "You have to 'cs' to a star to name it.\n";
      return;
    }
  } else if (argv[1] == "planet") {
    if (g.level == ScopeLevel::LEVEL_PLAN) {
      getstar(&Stars[g.snum], g.snum);
      Race = races[Playernum - 1];
      if (!Race->God) {
        g.out << "Only deity can rename planets.\n";
        return;
      }
      strncpy(Stars[g.snum]->pnames[g.pnum], buf, NAMESIZE - 1);
      putstar(Stars[g.snum], g.snum);
      deductAPs(Playernum, Governor, APcount, g.snum, 0);
    } else {
      g.out << "You have to 'cs' to a planet to name it.\n";
      return;
    }
  } else if (argv[1] == "race") {
    Race = races[Playernum - 1];
    if (Governor) {
      g.out << "You are not authorized to do this.\n";
      return;
    }
    strncpy(Race->name, buf, RNAMESIZE - 1);
    sprintf(buf, "Name changed to `%s'.\n", Race->name);
    notify(Playernum, Governor, buf);
    putrace(Race);
  } else if (argv[1] == "governor") {
    Race = races[Playernum - 1];
    strncpy(Race->governor[Governor].name, buf, RNAMESIZE - 1);
    sprintf(buf, "Name changed to `%s'.\n", Race->governor[Governor].name);
    notify(Playernum, Governor, buf);
    putrace(Race);
  } else {
    g.out << "I don't know what you mean.\n";
    return;
  }
}

