// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* name.c -- rename something to something else */

#include "name.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>

#include "GB_server.h"
#include "buffers.h"
#include "capture.h"
#include "dissolve.h"
#include "files.h"
#include "files_shl.h"
#include "getplace.h"
#include "max.h"
#include "mobiliz.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

static char msg[1024];

void personal(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  if (Governor) {
    notify(Playernum, Governor, "Only the leader can do this.\n");
    return;
  }
  auto Race = races[Playernum - 1];
  strncpy(Race->info, message.c_str(), PERSONALSIZE - 1);
  putrace(Race);
}

void bless(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  racetype *Race;
  int who, amount, Mod;
  char commod;

  Race = races[Playernum - 1];
  if (!Race->God) {
    notify(Playernum, Governor,
           "You are not privileged to use this command.\n");
    return;
  }
  if (g.level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor, "Please cs to the planet in question.\n");
    return;
  }
  who = atoi(argv[1].c_str());
  if (who < 1 || who > Num_races) {
    notify(Playernum, Governor, "No such player number.\n");
    return;
  }
  if (argv.size() < 3) {
    notify(Playernum, Governor, "Syntax: bless <player> <what> <+amount>\n");
    return;
  }
  amount = atoi(argv[3].c_str());

  Race = races[who - 1];
  /* race characteristics? */
  Mod = 1;

  if (match(argv[2].c_str(), "money")) {
    Race->governor[0].money += amount;
    sprintf(buf, "Deity gave you %d money.\n", amount);
  } else if (match(argv[2].c_str(), "password")) {
    strcpy(Race->password, argv[3].c_str());
    sprintf(buf, "Deity changed your race password to `%s'\n", argv[3].c_str());
  } else if (match(argv[2].c_str(), "morale")) {
    Race->morale += amount;
    sprintf(buf, "Deity gave you %d morale.\n", amount);
  } else if (match(argv[2].c_str(), "pods")) {
    Race->pods = 1;
    sprintf(buf, "Deity gave you pod ability.\n");
  } else if (match(argv[2].c_str(), "nopods")) {
    Race->pods = 0;
    sprintf(buf, "Deity took away pod ability.\n");
  } else if (match(argv[2].c_str(), "collectiveiq")) {
    Race->collective_iq = 1;
    sprintf(buf, "Deity gave you collective intelligence.\n");
  } else if (match(argv[2].c_str(), "nocollectiveiq")) {
    Race->collective_iq = 0;
    sprintf(buf, "Deity took away collective intelligence.\n");
  } else if (match(argv[2].c_str(), "maxiq")) {
    Race->IQ_limit = atoi(argv[3].c_str());
    sprintf(buf, "Deity gave you a maximum IQ of %d.\n", Race->IQ_limit);
  } else if (match(argv[2].c_str(), "mass")) {
    Race->mass = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f mass.\n", Race->mass);
  } else if (match(argv[2].c_str(), "metabolism")) {
    Race->metabolism = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f metabolism.\n", Race->metabolism);
  } else if (match(argv[2].c_str(), "adventurism")) {
    Race->adventurism = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %-3.0f%% adventurism.\n",
            Race->adventurism * 100.0);
  } else if (match(argv[2].c_str(), "birthrate")) {
    Race->birthrate = atof(argv[3].c_str());
    sprintf(buf, "Deity gave you %.2f birthrate.\n", Race->birthrate);
  } else if (match(argv[2].c_str(), "fertility")) {
    Race->fertilize = amount;
    sprintf(buf, "Deity gave you a fetilization ability of %d.\n", amount);
  } else if (match(argv[2].c_str(), "IQ")) {
    Race->IQ = amount;
    sprintf(buf, "Deity gave you %d IQ.\n", amount);
  } else if (match(argv[2].c_str(), "fight")) {
    Race->fighters = amount;
    sprintf(buf, "Deity set your fighting ability to %d.\n", amount);
  } else if (match(argv[2].c_str(), "technology")) {
    Race->tech += (double)amount;
    sprintf(buf, "Deity gave you %d technology.\n", amount);
  } else if (match(argv[2].c_str(), "guest")) {
    Race->Guest = 1;
    sprintf(buf, "Deity turned you into a guest race.\n");
  } else if (match(argv[2].c_str(), "god")) {
    Race->God = 1;
    sprintf(buf, "Deity turned you into a deity race.\n");
  } else if (match(argv[2].c_str(), "mortal")) {
    Race->God = 0;
    Race->Guest = 0;
    sprintf(buf, "Deity turned you into a mortal race.\n");
    /* sector preferences */
  } else if (match(argv[2].c_str(), "water")) {
    Race->likes[SectorType::SEC_SEA] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your water preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "land")) {
    Race->likes[SectorType::SEC_LAND] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your land preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "mountain")) {
    Race->likes[SectorType::SEC_MOUNT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your mountain preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "gas")) {
    Race->likes[SectorType::SEC_GAS] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your gas preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "ice")) {
    Race->likes[SectorType::SEC_ICE] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your ice preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "forest")) {
    Race->likes[SectorType::SEC_FOREST] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your forest preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "desert")) {
    Race->likes[SectorType::SEC_DESERT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your desert preference to %d%%\n", amount);
  } else if (match(argv[2].c_str(), "plated")) {
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
  commod = argv[2].c_str()[0];
  auto planet = getplanet(g.snum, g.pnum);
  if (match(argv[2].c_str(), "explorebit")) {
    planet.info[who - 1].explored = 1;
    getstar(&Stars[g.snum], g.snum);
    setbit(Stars[g.snum]->explored, who);
    putstar(Stars[g.snum], g.snum);
    sprintf(buf, "Deity set your explored bit at /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (match(argv[2].c_str(), "noexplorebit")) {
    planet.info[who - 1].explored = 0;
    sprintf(buf, "Deity reset your explored bit at /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (match(argv[2].c_str(), "planetpopulation")) {
    planet.info[who - 1].popn = atoi(argv[3].c_str());
    planet.popn++;
    sprintf(buf, "Deity set your population variable to %ld at /%s/%s.\n",
            planet.info[who - 1].popn, Stars[g.snum]->name,
            Stars[g.snum]->pnames[g.pnum]);
  } else if (match(argv[2].c_str(), "inhabited")) {
    getstar(&Stars[g.snum], g.snum);
    setbit(Stars[g.snum]->inhabited, Playernum);
    putstar(Stars[g.snum], g.snum);
    sprintf(buf, "Deity has set your inhabited bit for /%s/%s.\n",
            Stars[g.snum]->name, Stars[g.snum]->pnames[g.pnum]);
  } else if (match(argv[2].c_str(), "numsectsowned")) {
    planet.info[who - 1].numsectsowned = atoi(argv[3].c_str());
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
        notify(Playernum, Governor, "No such commodity.\n");
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
  int who, amount, eligible, them = 0;
  racetype *Race, *alien;
  double x;
  int changed_hands, chance;
  int i;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You must 'cs' to the planet you wish to try it on.\n");
    return;
  }
  if (!control(Playernum, Governor, Stars[g.snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  /*  if(argv.size()<3) {
        notify(Playernum, Governor, "The correct syntax is 'insurgency <race>
    <money>'\n");
        return;
    }*/
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount))
    return;
  if (!(who = GetPlayer(argv[1].c_str()))) {
    sprintf(buf, "No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  Race = races[Playernum - 1];
  alien = races[who - 1];
  if (alien->Guest) {
    notify(Playernum, Governor, "Don't be such a dickweed.\n");
    return;
  }
  if (who == Playernum) {
    notify(Playernum, Governor, "You can't revolt against yourself!\n");
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
    notify(
        Playernum, Governor,
        "You must have population in the star system to attempt insurgency\n.");
    return;
  }
  auto p = getplanet(g.snum, g.pnum);

  if (!p.info[who - 1].popn) {
    notify(Playernum, Governor, "This player does not occupy this planet.\n");
    return;
  }

  sscanf(argv[2].c_str(), "%d", &amount);
  if (amount < 0) {
    notify(Playernum, Governor,
           "You have to use a positive amount of money.\n");
    return;
  }
  if (Race->governor[Governor].money < amount) {
    notify(Playernum, Governor, "Nice try.\n");
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
    changed_hands = revolt(&p, who, Playernum);
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
    notify(Playernum, Governor, "The insurgency failed!\n");
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
  int who, amount;
  racetype *Race, *alien;

  if (!(who = GetPlayer(argv[1].c_str()))) {
    sprintf(buf, "No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (Governor) {
    notify(Playernum, Governor, "You are not authorized to do that.\n");
    return;
  }
  Race = races[Playernum - 1];
  alien = races[who - 1];

  sscanf(argv[2].c_str(), "%d", &amount);
  if (amount < 0) {
    notify(Playernum, Governor,
           "You have to give a player a positive amount of money.\n");
    return;
  }
  if (Race->Guest) {
    notify(Playernum, Governor,
           "Nice try. Your attempt has been duly noted.\n");
    return;
  }
  if (Race->governor[Governor].money < amount) {
    notify(Playernum, Governor, "You don't have that much money to give!\n");
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
  int who, sh;
  shiptype *ship;
  racetype *Race, *alien;

  if (!(who = GetPlayer(argv[1].c_str()))) {
    sprintf(buf, "No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (Governor) {
    notify(Playernum, Governor, "You are not authorized to do that.\n");
    return;
  }
  alien = races[who - 1];
  Race = races[Playernum - 1];
  if (alien->Guest && !Race->God) {
    notify(Playernum, Governor, "You can't give this player anything.\n");
    return;
  }
  if (Race->Guest) {
    notify(Playernum, Governor, "You can't give anyone anything.\n");
    return;
  }
  /* check to see if both players are mutually allied */
  if (!Race->God &&
      !(isset(Race->allied, who) && isset(alien->allied, Playernum))) {
    notify(Playernum, Governor, "You two are not mutually allied.\n");
    return;
  }
  sscanf(argv[2].c_str() + (argv[2].c_str()[0] == '#'), "%d", &sh);

  if (!getship(&ship, sh)) {
    notify(Playernum, Governor, "Illegal ship number.\n");
    return;
  }

  if (ship->owner != Playernum || !ship->alive) {
    DontOwnErr(Playernum, Governor, sh);
    free(ship);
    return;
  }
  if (ship->type == STYPE_POD) {
    notify(Playernum, Governor,
           "You cannot change the ownership of spore pods.\n");
    free(ship);
    return;
  }

  if ((ship->popn + ship->troops) && !Race->God) {
    notify(Playernum, Governor,
           "You can't give this ship away while it has crew/mil on board.\n");
    free(ship);
    return;
  }
  if (ship->ships && !Race->God) {
    notify(Playernum, Governor,
           "You can't give away this ship, it has other ships loaded on it.\n");
    free(ship);
    return;
  }
  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
        free(ship);
        return;
      }
      break;
    default:
      if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1],
                  APcount)) {
        free(ship);
        return;
      }
      break;
  }

  ship->owner = who;
  ship->governor = 0; /* give to the leader */
  capture_stuff(ship);

  putship(ship);

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
      notify(Playernum, Governor, "Something wrong with this ship's scope.\n");
      free(ship);
      return;
  }

  switch (ship->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      deductAPs(Playernum, Governor, APcount, 0, 1);
      free(ship);
      return;
    default:
      deductAPs(Playernum, Governor, APcount, g.snum, 0);
      break;
  }
  notify(Playernum, Governor, "Owner changed.\n");
  sprintf(buf, "%s [%d] gave you %s at %s.\n", Race->name, Playernum,
          Ship(*ship).c_str(), prin_ship_orbits(ship));
  warn(who, 0, buf);

  if (!Race->God) {
    sprintf(buf, "%s [%d] gives %s [%d] a ship.\n", Race->name, Playernum,
            alien->name, who);
    post(buf, TRANSFER);
    free(ship);
  }
}

void page(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = g.god ? 0 : 1;
  int i, who, gov, to_block, dummy[2];
  racetype *Race, *alien;

  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount))
    return;

  gov = 0;  // TODO(jeffbailey): Init to zero.
  to_block = 0;
  if (match(argv[1].c_str(), "block")) {
    to_block = 1;
    notify(Playernum, Governor, "Paging alliance block.\n");
    who = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
    gov = 0;  // TODO(jeffbailey): Init to zero to be sure it's initialized.
  } else {
    if (!(who = GetPlayer(argv[1].c_str()))) {
      sprintf(buf, "No such player.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
    if (argv.size() > 1) gov = atoi(argv[2].c_str());
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      sprintf(buf, "You can't make pages at universal scope.\n");
      notify(Playernum, Governor, buf);
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
        dummy[0] =
            Blocks[Playernum - 1].invite[0] & Blocks[Playernum - 1].pledge[0];
        dummy[1] =
            Blocks[Playernum - 1].invite[1] & Blocks[Playernum - 1].pledge[1];
        for (i = 1; i <= Num_races; i++)
          if (isset(dummy, i) && i != Playernum) notify_race(i, buf);
      } else {
        if (argv.size() > 1)
          notify(who, gov, buf);
        else
          notify_race(who, buf);
      }

      notify(Playernum, Governor, "Request sent.\n");
      break;
  }
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
}

void send_message(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  bool postit = argv[0] == "post" ? true : false;
  int APcount;
  if (postit) {
    APcount = 0;
  } else {
    APcount = g.god ? 0 : 1;
  }
  int who, i, j, to_block, dummy[2];
  int to_star, star, start;
  placetype where;
  racetype *Race, *alien;

  star = 0;  // TODO(jeffbailey): Init to zero.
  who = 0;   // TODO(jeffbailey): Init to zero.

  to_star = to_block = 0;

  if (argv.size() < 2) {
    notify(Playernum, Governor, "Send what?\n");
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
  if (match(argv[1].c_str(), "block")) {
    to_block = 1;
    notify(Playernum, Governor, "Sending message to alliance block.\n");
    if (!(who = GetPlayer(argv[2].c_str()))) {
      sprintf(buf, "No such alliance block.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
  } else if (match(argv[1].c_str(), "star")) {
    to_star = 1;
    notify(Playernum, Governor, "Sending message to star system.\n");
    where = Getplace(g, argv[2].c_str(), 1);
    if (where.err || where.level != ScopeLevel::LEVEL_STAR) {
      sprintf(buf, "No such star.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    star = where.snum;
    getstar(&(Stars[star]), star);
  } else {
    if (!(who = GetPlayer(argv[1].c_str()))) {
      sprintf(buf, "No such player.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      sprintf(buf, "You can't send messages from universal scope.\n");
      notify(Playernum, Governor, buf);
      return;

    case ScopeLevel::LEVEL_SHIP:
      sprintf(buf, "You can't send messages from ship scope.\n");
      notify(Playernum, Governor, buf);
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
    dummy[0] = (Blocks[who - 1].invite[0] & Blocks[who - 1].pledge[0]);
    dummy[1] = (Blocks[who - 1].invite[1] & Blocks[who - 1].pledge[1]);
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
    if (isdigit(*argv[2].c_str()) && (gov = atoi(argv[2].c_str())) >= 0 &&
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
        MIN(alien->translate[Playernum - 1] + 2, 100);
    putrace(alien);
  }
  notify(Playernum, Governor, "Message sent.\n");
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
}

void read_messages(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (argv.size() == 1 || match(argv[1].c_str(), "telegram"))
    teleg_read(Playernum, Governor);
  else if (match(argv[1].c_str(), "news")) {
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
    notify(Playernum, Governor, "Read what?\n");
}

void motto(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  if (Governor) {
    notify(Playernum, Governor, "You are not authorized to do this.\n");
    return;
  }
  strncpy(Blocks[Playernum - 1].motto, message.c_str(), MOTTOSIZE - 1);
  Putblock(Blocks);
  notify(Playernum, Governor, "Done.\n");
}

void name(const command_t &argv, GameObj &g) {
  int APcount = 0;
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  char *ch;
  int spaces;
  unsigned char check = 0;
  shiptype *ship;
  char string[1024];
  char temp[128];
  racetype *Race;

  if (!isalnum(argv[2].c_str()[0]) || argv.size() < 3) {
    notify(Playernum, Governor, "Illegal name format.\n");
    return;
  }

  sprintf(buf, "%s", argv[2].c_str());
  for (int i = 3; i < argv.size(); i++) {
    sprintf(temp, " %s", argv[i].c_str());
    strcat(buf, temp);
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
    notify(Playernum, Governor, "Illegal name.\n");
    return;
  }

  if (strlen(buf) < 1 || check) {
    sprintf(buf, "Illegal name %s.\n", check ? "form" : "length");
    notify(Playernum, Governor, buf);
    return;
  }

  if (match(argv[1].c_str(), "ship")) {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      (void)getship(&ship, g.shipno);
      strncpy(ship->name, buf, SHIP_NAMESIZE);
      putship(ship);
      notify(Playernum, Governor, "Name set.\n");
      free(ship);
      return;
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a ship to name it.\n");
      return;
    }
  } else if (match(argv[1].c_str(), "class")) {
    if (g.level == ScopeLevel::LEVEL_SHIP) {
      (void)getship(&ship, g.shipno);
      if (ship->type != OTYPE_FACTORY) {
        notify(Playernum, Governor, "You are not at a factory!\n");
        free(ship);
        return;
      }
      if (ship->on) {
        notify(Playernum, Governor, "This factory is already on line.\n");
        free(ship);
        return;
      }
      strncpy(ship->shipclass, buf, SHIP_NAMESIZE - 1);
      putship(ship);
      notify(Playernum, Governor, "Class set.\n");
      free(ship);
      return;
    } else {
      notify(Playernum, Governor,
             "You have to 'cs' to a factory to name the ship class.\n");
      return;
    }
  } else if (match(argv[1].c_str(), "block")) {
    /* name your alliance block */
    if (Governor) {
      notify(Playernum, Governor, "You are not authorized to do this.\n");
      return;
    }
    strncpy(Blocks[Playernum - 1].name, buf, RNAMESIZE - 1);
    Putblock(Blocks);
    notify(Playernum, Governor, "Done.\n");
  } else if (match(argv[1].c_str(), "star")) {
    if (g.level == ScopeLevel::LEVEL_STAR) {
      Race = races[Playernum - 1];
      if (!Race->God) {
        notify(Playernum, Governor, "Only dieties may name a star.\n");
        return;
      }
      strncpy(Stars[g.snum]->name, buf, NAMESIZE - 1);
      putstar(Stars[g.snum], g.snum);
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a star to name it.\n");
      return;
    }
  } else if (match(argv[1].c_str(), "planet")) {
    if (g.level == ScopeLevel::LEVEL_PLAN) {
      getstar(&Stars[g.snum], g.snum);
      Race = races[Playernum - 1];
      if (!Race->God) {
        notify(Playernum, Governor, "Only deity can rename planets.\n");
        return;
      }
      strncpy(Stars[g.snum]->pnames[g.pnum], buf, NAMESIZE - 1);
      putstar(Stars[g.snum], g.snum);
      deductAPs(Playernum, Governor, APcount, g.snum, 0);
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a planet to name it.\n");
      return;
    }
  } else if (match(argv[1].c_str(), "race")) {
    Race = races[Playernum - 1];
    if (Governor) {
      notify(Playernum, Governor, "You are not authorized to do this.\n");
      return;
    }
    strncpy(Race->name, buf, RNAMESIZE - 1);
    sprintf(buf, "Name changed to `%s'.\n", Race->name);
    notify(Playernum, Governor, buf);
    putrace(Race);
  } else if (match(argv[1].c_str(), "governor")) {
    Race = races[Playernum - 1];
    strncpy(Race->governor[Governor].name, buf, RNAMESIZE - 1);
    sprintf(buf, "Name changed to `%s'.\n", Race->governor[Governor].name);
    notify(Playernum, Governor, buf);
    putrace(Race);
  } else {
    notify(Playernum, Governor, "I don't know what you mean.\n");
    return;
  }
}

void announce(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  enum class Communicate {
    ANN,
    BROADCAST,
    SHOUT,
    THINK,
  };

  Communicate mode;
  if (argv[0] == "announce")
    mode = Communicate::ANN;
  else if (argv[0] == "broadcast")
    mode = Communicate::BROADCAST;
  else if (argv[0] == "'")
    mode = Communicate::BROADCAST;
  else if (argv[0] == "shout")
    mode = Communicate::SHOUT;
  else if (argv[0] == "think")
    mode = Communicate::THINK;
  else {
    notify(Playernum, Governor, "Not sure how you got here.\n");
    return;
  }

  racetype *Race;
  char symbol;

  Race = races[Playernum - 1];
  if (mode == Communicate::SHOUT && !Race->God) {
    notify(Playernum, Governor,
           "You are not privileged to use this command.\n");
    return;
  }

  std::stringstream ss_message;
  std::copy(++argv.begin(), argv.end(),
            std::ostream_iterator<std::string>(ss_message, " "));
  std::string message = ss_message.str();

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (mode == Communicate::ANN) mode = Communicate::BROADCAST;
      break;
    default:
      if ((mode == Communicate::ANN) &&
          !(!!isset(Stars[g.snum]->inhabited, Playernum) || Race->God)) {
        sprintf(buf,
                "You do not inhabit this system or have diety privileges.\n");
        notify(Playernum, Governor, buf);
        return;
      }
  }

  switch (mode) {
    case Communicate::ANN:
      symbol = ':';
      break;
    case Communicate::BROADCAST:
      symbol = '>';
      break;
    case Communicate::SHOUT:
      symbol = '!';
      break;
    case Communicate::THINK:
      symbol = '=';
      break;
  }
  sprintf(msg, "%s \"%s\" [%d,%d] %c %s\n", Race->name,
          Race->governor[Governor].name, Playernum, Governor, symbol,
          message.c_str());

  switch (mode) {
    case Communicate::ANN:
      d_announce(Playernum, Governor, g.snum, msg);
      break;
    case Communicate::BROADCAST:
      d_broadcast(Playernum, Governor, msg);
      break;
    case Communicate::SHOUT:
      d_shout(Playernum, Governor, msg);
      break;
    case Communicate::THINK:
      d_think(Playernum, Governor, msg);
      break;
  }
}
