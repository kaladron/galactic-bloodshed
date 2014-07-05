/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *
 * name.c -- rename something to something else
* announce.c -- make announcements in the system you currently in.
*      You must be inhabiting that system for your message to sent.
*      You must also be in that system (and inhabiting) to receive
*announcements.
* page.c -- send a message to a player requesting his presence in a system.
*/
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "races.h"
#include "power.h"
#include "ships.h"
#include "buffers.h"

char msg[1024];
struct tm *current_tm; /* for watching for next update */
long clk;

void personal(int, int, char *);
void bless(int, int, int);
void insurgency(int, int, int);
void pay(int, int, int);
void give(int, int, int);
void page(int, int, int);
void send_message(int, int, int, int);
void read_messages(int, int, int);
void motto(int, int, int, char *);
void name(int, int, int);
int MostAPs(int, startype *);
void announce(int, int, char *, int);
#include "getplace.p"
#include "GB_server.p"
#include "files_shl.p"
#include "shlmisc.p"
#include "max.p"
#include "mobiliz.p"
#include "dissolve.p"
#include "teleg_send.p"
#include "capture.p"
#include "rand.p"
#include "read_teleg.p"

void personal(int Playernum, int Governor, char *message) {
  racetype *Race;

  if (Governor) {
    notify(Playernum, Governor, "Only the leader can do this.\n");
    return;
  }
  Race = races[Playernum - 1];
  strncpy(Race->info, message, PERSONALSIZE - 1);
  putrace(Race);
}

void bless(int Playernum, int Governor, int APcount) {
  planettype *planet;
  racetype *Race;
  int who, amount, Mod;
  char commod;

  Race = races[Playernum - 1];
  if (!Race->God) {
    notify(Playernum, Governor,
           "You are not privileged to use this command.\n");
    return;
  }
  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    notify(Playernum, Governor, "Please cs to the planet in question.\n");
    return;
  }
  who = atoi(args[1]);
  if (who < 1 || who > Num_races) {
    notify(Playernum, Governor, "No such player number.\n");
    return;
  }
  if (argn < 3) {
    notify(Playernum, Governor, "Syntax: bless <player> <what> <+amount>\n");
    return;
  }
  amount = atoi(args[3]);

  Race = races[who - 1];
  /* race characteristics? */
  Mod = 1;

  if (match(args[2], "money")) {
    Race->governor[0].money += amount;
    sprintf(buf, "Deity gave you %d money.\n", amount);
  } else if (match(args[2], "password")) {
    strcpy(Race->password, args[3]);
    sprintf(buf, "Deity changed your race password to `%s'\n", args[3]);
  } else if (match(args[2], "morale")) {
    Race->morale += amount;
    sprintf(buf, "Deity gave you %d morale.\n", amount);
  } else if (match(args[2], "pods")) {
    Race->pods = 1;
    sprintf(buf, "Deity gave you pod ability.\n");
  } else if (match(args[2], "nopods")) {
    Race->pods = 0;
    sprintf(buf, "Deity took away pod ability.\n");
  } else if (match(args[2], "collectiveiq")) {
    Race->collective_iq = 1;
    sprintf(buf, "Deity gave you collective intelligence.\n");
  } else if (match(args[2], "nocollectiveiq")) {
    Race->collective_iq = 0;
    sprintf(buf, "Deity took away collective intelligence.\n");
  } else if (match(args[2], "maxiq")) {
    Race->IQ_limit = atoi(args[3]);
    sprintf(buf, "Deity gave you a maximum IQ of %d.\n", Race->IQ_limit);
  } else if (match(args[2], "mass")) {
    Race->mass = atof(args[3]);
    sprintf(buf, "Deity gave you %.2f mass.\n", Race->mass);
  } else if (match(args[2], "metabolism")) {
    Race->metabolism = atof(args[3]);
    sprintf(buf, "Deity gave you %.2f metabolism.\n", Race->metabolism);
  } else if (match(args[2], "adventurism")) {
    Race->adventurism = atof(args[3]);
    sprintf(buf, "Deity gave you %-3.0f%% adventurism.\n",
            Race->adventurism * 100.0);
  } else if (match(args[2], "birthrate")) {
    Race->birthrate = atof(args[3]);
    sprintf(buf, "Deity gave you %.2f birthrate.\n", Race->birthrate);
  } else if (match(args[2], "fertility")) {
    Race->fertilize = amount;
    sprintf(buf, "Deity gave you a fetilization ability of %d.\n", amount);
  } else if (match(args[2], "IQ")) {
    Race->IQ = amount;
    sprintf(buf, "Deity gave you %d IQ.\n", amount);
  } else if (match(args[2], "fight")) {
    Race->fighters = amount;
    sprintf(buf, "Deity set your fighting ability to %d.\n", amount);
  } else if (match(args[2], "technology")) {
    Race->tech += (double)amount;
    sprintf(buf, "Deity gave you %d technology.\n", amount);
  } else if (match(args[2], "guest")) {
    Race->Guest = 1;
    sprintf(buf, "Deity turned you into a guest race.\n");
  } else if (match(args[2], "god")) {
    Race->God = 1;
    sprintf(buf, "Deity turned you into a deity race.\n");
  } else if (match(args[2], "mortal")) {
    Race->God = 0;
    Race->Guest = 0;
    sprintf(buf, "Deity turned you into a mortal race.\n");
    /* sector preferences */
  } else if (match(args[2], "water")) {
    Race->likes[SEA] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your water preference to %d%%\n", amount);
  } else if (match(args[2], "land")) {
    Race->likes[LAND] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your land preference to %d%%\n", amount);
  } else if (match(args[2], "mountain")) {
    Race->likes[MOUNT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your mountain preference to %d%%\n", amount);
  } else if (match(args[2], "gas")) {
    Race->likes[GAS] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your gas preference to %d%%\n", amount);
  } else if (match(args[2], "ice")) {
    Race->likes[ICE] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your ice preference to %d%%\n", amount);
  } else if (match(args[2], "forest")) {
    Race->likes[FOREST] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your forest preference to %d%%\n", amount);
  } else if (match(args[2], "desert")) {
    Race->likes[DESERT] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your desert preference to %d%%\n", amount);
  } else if (match(args[2], "plated")) {
    Race->likes[PLATED] = 0.01 * (double)amount;
    sprintf(buf, "Deity set your plated preference to %d%%\n", amount);
  } else
    Mod = 0;
  if (Mod) {
    putrace(Race);
    warn(who, 0, buf);
  }
  if (Mod)
    return;
  /* ok, must be the planet then */
  commod = args[2][0];
  getplanet(&planet, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  if (match(args[2], "explorebit")) {
    planet->info[who - 1].explored = 1;
    getstar(&Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    setbit(Stars[Dir[Playernum - 1][Governor].snum]->explored, who);
    putstar(Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    sprintf(buf, "Deity set your explored bit at /%s/%s.\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum]);
  } else if (match(args[2], "noexplorebit")) {
    planet->info[who - 1].explored = 0;
    sprintf(buf, "Deity reset your explored bit at /%s/%s.\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum]);
  } else if (match(args[2], "planetpopulation")) {
    planet->info[who - 1].popn = atoi(args[3]);
    planet->popn++;
    sprintf(buf, "Deity set your population variable to %ld at /%s/%s.\n",
            planet->info[who - 1].popn,
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum]);
  } else if (match(args[2], "inhabited")) {
    getstar(&Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    setbit(Stars[Dir[Playernum - 1][Governor].snum]->inhabited, Playernum);
    putstar(Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    sprintf(buf, "Deity has set your inhabited bit for /%s/%s.\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum]);
  } else if (match(args[2], "numsectsowned")) {
    planet->info[who - 1].numsectsowned = atoi(args[3]);
    sprintf(buf, "Deity set your \"numsectsowned\" variable at /%s/%s to %d.\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum],
            planet->info[who - 1].numsectsowned);
  } else {
    switch (commod) {
    case 'r':
      planet->info[who - 1].resource += amount;
      sprintf(buf, "Deity gave you %d resources at %s/%s.\n", amount,
              Stars[Dir[Playernum - 1][Governor].snum]->name,
              Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum]);
      break;
    case 'd':
      planet->info[who - 1].destruct += amount;
      sprintf(buf, "Deity gave you %d destruct at %s/%s.\n", amount,
              Stars[Dir[Playernum - 1][Governor].snum]->name,
              Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum]);
      break;
    case 'f':
      planet->info[who - 1].fuel += amount;
      sprintf(buf, "Deity gave you %d fuel at %s/%s.\n", amount,
              Stars[Dir[Playernum - 1][Governor].snum]->name,
              Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum]);
      break;
    case 'x':
      planet->info[who - 1].crystals += amount;
      sprintf(buf, "Deity gave you %d crystals at %s/%s.\n", amount,
              Stars[Dir[Playernum - 1][Governor].snum]->name,
              Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum]);
      break;
    case 'a':
      getstar(&Stars[Dir[Playernum - 1][Governor].snum],
              Dir[Playernum - 1][Governor].snum);
      Stars[Dir[Playernum - 1][Governor].snum]->AP[who - 1] += amount;
      putstar(Stars[Dir[Playernum - 1][Governor].snum],
              Dir[Playernum - 1][Governor].snum);
      sprintf(buf, "Deity gave you %d action points at %s.\n", amount,
              Stars[Dir[Playernum - 1][Governor].snum]->name);
      break;
    default:
      notify(Playernum, Governor, "No such commodity.\n");
      free(planet);
      return;
    }
  }
  putplanet(planet, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  warn_race(who, buf);
  free(planet);
}

void insurgency(int Playernum, int Governor, int APcount) {
  int who, amount, eligible, them = 0;
  racetype *Race, *alien;
  planettype *p;
  double x;
  int changed_hands, chance;
  register int i;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You must 'cs' to the planet you wish to try it on.\n");
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  /*  if(argn<3) {
        notify(Playernum, Governor, "The correct syntax is 'insurgency <race>
    <money>'\n");
        return;
    }*/
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount))
    return;
  if (!(who = GetPlayer(args[1]))) {
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
  for (i = 0; i < Stars[Dir[Playernum - 1][Governor].snum]->numplanets; i++) {
    getplanet(&p, Dir[Playernum - 1][Governor].snum, i);
    eligible += p->info[Playernum - 1].popn;
    them += p->info[who - 1].popn;
    free(p);
  }
  if (!eligible) {
    notify(
        Playernum, Governor,
        "You must have population in the star system to attempt insurgency\n.");
    return;
  }
  getplanet(&p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  if (!p->info[who - 1].popn) {
    notify(Playernum, Governor, "This player does not occupy this planet.\n");
    free(p);
    return;
  }

  sscanf(args[2], "%d", &amount);
  if (amount < 0) {
    notify(Playernum, Governor,
           "You have to use a positive amount of money.\n");
    free(p);
    return;
  }
  if (Race->governor[Governor].money < amount) {
    notify(Playernum, Governor, "Nice try.\n");
    free(p);
    return;
  }

  x = INSURG_FACTOR * (double)amount * (double)p->info[who - 1].tax /
      (double)p->info[who - 1].popn;
  x *= morale_factor((double)(Race->morale - alien->morale));
  x *= morale_factor((double)(eligible - them) / 50.0);
  x *= morale_factor(10.0 *
                     (double)(Race->fighters * p->info[Playernum - 1].troops -
                              alien->fighters * p->info[who - 1].troops)) /
       50.0;
  sprintf(buf, "x = %f\n", x);
  notify(Playernum, Governor, buf);
  chance = round_rand(200.0 * atan((double)x) / 3.14159265);
  sprintf(long_buf, "%s/%s: %s [%d] tries insurgency vs %s [%d]\n",
          Stars[Dir[Playernum - 1][Governor].snum]->name,
          Stars[Dir[Playernum - 1][Governor].snum]
              ->pnames[Dir[Playernum - 1][Governor].pnum],
          Race->name, Playernum, alien->name, who);
  sprintf(buf, "\t%s: %d total civs [%d]  opposing %d total civs [%d]\n",
          Stars[Dir[Playernum - 1][Governor].snum]->name, eligible, Playernum,
          them, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %ld morale [%d] vs %ld morale [%d]\n", Race->morale,
          Playernum, alien->morale, who);
  strcat(long_buf, buf);
  sprintf(buf, "\t\t %d money against %ld population at tax rate %d%%\n",
          amount, p->info[who - 1].popn, p->info[who - 1].tax);
  strcat(long_buf, buf);
  sprintf(buf, "Success chance is %d%%\n", chance);
  strcat(long_buf, buf);
  if (success(chance)) {
    changed_hands = revolt(p, who, Playernum);
    notify(Playernum, Governor, long_buf);
    sprintf(buf, "Success!  You liberate %d sector%s.\n", changed_hands,
            (changed_hands == 1) ? "" : "s");
    notify(Playernum, Governor, buf);
    sprintf(
        buf, "A revolt on /%s/%s instigated by %s [%d] costs you %d sector%s\n",
        Stars[Dir[Playernum - 1][Governor].snum]->name,
        Stars[Dir[Playernum - 1][Governor].snum]
            ->pnames[Dir[Playernum - 1][Governor].pnum],
        Race->name, Playernum, changed_hands, (changed_hands == 1) ? "" : "s");
    strcat(long_buf, buf);
    warn(who, (int)Stars[Dir[Playernum - 1][Governor].snum]->governor[who - 1],
         long_buf);
    p->info[Playernum - 1].tax = p->info[who - 1].tax;
    /* you inherit their tax rate (insurgency wars he he ) */
    sprintf(buf, "/%s/%s: Successful insurgency by %s [%d] against %s [%d]\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum],
            Race->name, Playernum, alien->name, who);
    post(buf, DECLARATION);
  } else {
    notify(Playernum, Governor, long_buf);
    notify(Playernum, Governor, "The insurgency failed!\n");
    sprintf(buf, "A revolt on /%s/%s instigated by %s [%d] fails\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum],
            Race->name, Playernum);
    strcat(long_buf, buf);
    warn(who, (int)Stars[Dir[Playernum - 1][Governor].snum]->governor[who - 1],
         long_buf);
    sprintf(buf, "/%s/%s: Failed insurgency by %s [%d] against %s [%d]\n",
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum],
            Race->name, Playernum, alien->name, who);
    post(buf, DECLARATION);
  }
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
  Race->governor[Governor].money -= amount;
  putrace(Race);
  free(p);
}

void pay(int Playernum, int Governor, int APcount) {
  int who, amount;
  racetype *Race, *alien;

  if (!(who = GetPlayer(args[1]))) {
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

  sscanf(args[2], "%d", &amount);
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

void give(int Playernum, int Governor, int APcount) {
  int who, sh;
  shiptype *ship;
  planettype *planet;
  racetype *Race, *alien;

  if (!(who = GetPlayer(args[1]))) {
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
  sscanf(args[2] + (args[2][0] == '#'), "%d", &sh);

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
  case LEVEL_UNIV:
    if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
      free(ship);
      return;
    }
    break;
  default:
    if (!enufAP(Playernum, Governor,
                Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
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
  case LEVEL_UNIV:
    break;
  case LEVEL_STAR:
    getstar(&(Stars[ship->storbits]), (int)ship->storbits);
    setbit(Stars[ship->storbits]->explored, who);
    putstar(Stars[ship->storbits], (int)ship->storbits);
    break;
  case LEVEL_PLAN:
    getstar(&(Stars[ship->storbits]), (int)ship->storbits);
    setbit(Stars[ship->storbits]->explored, who);
    putstar(Stars[ship->storbits], (int)ship->storbits);

    getplanet(&planet, (int)ship->storbits, (int)ship->pnumorbits);
    planet->info[who - 1].explored = 1;
    putplanet(planet, (int)ship->storbits, (int)ship->pnumorbits);
    free(planet);

    break;
  default:
    notify(Playernum, Governor, "Something wrong with this ship's scope.\n");
    free(ship);
    return;
    break;
  }

  switch (ship->whatorbits) {
  case LEVEL_UNIV:
    deductAPs(Playernum, Governor, APcount, 0, 1);
    free(ship);
    return;
    break;
  default:
    deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum,
              0);
    break;
  }
  notify(Playernum, Governor, "Owner changed.\n");
  sprintf(buf, "%s [%d] gave you %s at %s.\n", Race->name, Playernum,
          Ship(ship), prin_ship_orbits(ship));
  warn(who, 0, buf);

  if (!Race->God) {
    sprintf(buf, "%s [%d] gives %s [%d] a ship.\n", Race->name, Playernum,
            alien->name, who);
    post(buf, TRANSFER);
    free(ship);
  }
}

void page(int Playernum, int Governor, int APcount0) {
  int i, who, gov, to_block, dummy[2], APcount;
  racetype *Race, *alien;

  APcount = APcount0;
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount))
    return;

  gov = 0; // TODO(jeffbailey): Init to zero.
  to_block = 0;
  if (match(args[1], "block")) {
    to_block = 1;
    notify(Playernum, Governor, "Paging alliance block.\n");
    who = 0; // TODO(jeffbailey): Init to zero to be sure it's initialized.
    gov = 0; // TODO(jeffbailey): Init to zero to be sure it's initialized.
  } else {
    if (!(who = GetPlayer(args[1]))) {
      sprintf(buf, "No such player.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
    if (argn > 1)
      gov = atoi(args[2]);
  }

  switch (Dir[Playernum - 1][Governor].level) {
  case LEVEL_UNIV:
    sprintf(buf, "You can't make pages at universal scope.\n");
    notify(Playernum, Governor, buf);
    break;
  default:
    getstar(&Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    if (!enufAP(Playernum, Governor,
                Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
                APcount)) {
      return;
    }

    Race = races[Playernum - 1];

    sprintf(buf, "%s \"%s\" page(s) you from the %s star system.\n", Race->name,
            Race->governor[Governor].name,
            Stars[Dir[Playernum - 1][Governor].snum]->name);

    if (to_block) {
      dummy[0] =
          Blocks[Playernum - 1].invite[0] & Blocks[Playernum - 1].pledge[0];
      dummy[1] =
          Blocks[Playernum - 1].invite[1] & Blocks[Playernum - 1].pledge[1];
      for (i = 1; i <= Num_races; i++)
        if (isset(dummy, i) && i != Playernum)
          notify_race(i, buf);
    } else {
      if (argn > 1)
        notify(who, gov, buf);
      else
        notify_race(who, buf);
    }

    notify(Playernum, Governor, "Request sent.\n");
    break;
  }
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
}

void send_message(int Playernum, int Governor, int APcount0, int postit) {
  int who, i, j, to_block, dummy[2], APcount;
  int to_star, star, start;
  placetype where;
  racetype *Race, *alien;

  APcount = APcount0;

  star = 0; // TODO(jeffbailey): Init to zero.
  who = 0;  // TODO(jeffbailey): Init to zero.

  to_star = to_block = 0;

  if (argn < 2) {
    notify(Playernum, Governor, "Send what?\n");
    return;
  }
  if (postit) {
    Race = races[Playernum - 1];
    sprintf(msg, "%s \"%s\" [%d,%d]: ", Race->name,
            Race->governor[Governor].name, Playernum, Governor);
    /* put the message together */
    for (j = 1; j < argn; j++) {
      sprintf(buf, "%s ", args[j]);
      strcat(msg, buf);
    }
    strcat(msg, "\n");
    post(msg, ANNOUNCE);
    return;
  }
  if (match(args[1], "block")) {
    to_block = 1;
    notify(Playernum, Governor, "Sending message to alliance block.\n");
    if (!(who = GetPlayer(args[2]))) {
      sprintf(buf, "No such alliance block.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
  } else if (match(args[1], "star")) {
    to_star = 1;
    notify(Playernum, Governor, "Sending message to star system.\n");
    where = Getplace(Playernum, Governor, args[2], 1);
    if (where.err || where.level != LEVEL_STAR) {
      sprintf(buf, "No such star.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    star = where.snum;
    getstar(&(Stars[star]), star);
  } else {
    if (!(who = GetPlayer(args[1]))) {
      sprintf(buf, "No such player.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    alien = races[who - 1];
    APcount *= !alien->God;
  }

  switch (Dir[Playernum - 1][Governor].level) {
  case LEVEL_UNIV:
    sprintf(buf, "You can't send messages from universal scope.\n");
    notify(Playernum, Governor, buf);
    return;
    break;

  case LEVEL_SHIP:
    sprintf(buf, "You can't send messages from ship scope.\n");
    notify(Playernum, Governor, buf);
    return;
    break;

  default:
    getstar(&Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].snum);
    if (!enufAP(Playernum, Governor,
                Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
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

  if (to_star || to_block || isdigit(*args[2]))
    start = 3;
  else if (postit)
    start = 1;
  else
    start = 2;
  /* put the message together */
  for (j = start; j < argn; j++) {
    sprintf(buf, "%s ", args[j]);
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
    notify_star(Playernum, Governor, 0, star, buf);
    warn_star(Playernum, 0, star, msg);
  } else {
    int gov;
    if (who == Playernum)
      APcount = 0;
    if (isdigit(*args[2]) && (gov = atoi(args[2])) >= 0 &&
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
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
}

void read_messages(int Playernum, int Governor, int APcount) {
  if (argn == 1 || match(args[1], "telegram"))
    teleg_read(Playernum, Governor);
  else if (match(args[1], "news")) {
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

void motto(int Playernum, int Governor, int APcount, char *message) {
  if (Governor) {
    notify(Playernum, Governor, "You are not authorized to do this.\n");
    return;
  }
  strncpy(Blocks[Playernum - 1].motto, message, MOTTOSIZE - 1);
  Putblock(Blocks);
  notify(Playernum, Governor, "Done.\n");
}

void name(int Playernum, int Governor, int APcount) {
  char *ch;
  register int i, spaces;
  int len;
  unsigned char check = 0;
  shiptype *ship;
  char string[1024];
  char temp[128];
  racetype *Race;

  if (!isalnum(args[2][0]) || argn < 3) {
    notify(Playernum, Governor, "Illegal name format.\n");
    return;
  }

  sprintf(buf, "%s", args[2]);
  for (i = 3; i < argn; i++) {
    sprintf(temp, " %s", args[i]);
    strcat(buf, temp);
  }

  sprintf(string, "%s", buf);

  i = strlen(args[0]);

  /* make sure there are no ^'s or '/' in name,
    also make sure the name has at least 1 character in it */
  ch = string;
  spaces = 0;
  while (*ch != '\0') {
    check |=
        ((!isalnum(*ch) && !(*ch == ' ') && !(*ch == '.')) || (*ch == '/'));
    ch++;
    if (*ch == ' ')
      spaces++;
  }

  len = strlen(buf);
  if (spaces == strlen(buf)) {
    notify(Playernum, Governor, "Illegal name.\n");
    return;
  }

  if (strlen(buf) < 1 || check) {
    sprintf(buf, "Illegal name %s.\n", check ? "form" : "length");
    notify(Playernum, Governor, buf);
    return;
  }

  if (match(args[1], "ship")) {
    if (Dir[Playernum - 1][Governor].level == LEVEL_SHIP) {
      (void)getship(&ship, Dir[Playernum - 1][Governor].shipno);
      strncpy(ship->name, buf, SHIP_NAMESIZE);
      putship(ship);
      notify(Playernum, Governor, "Name set.\n");
      free(ship);
      return;
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a ship to name it.\n");
      return;
    }
  } else if (match(args[1], "class")) {
    if (Dir[Playernum - 1][Governor].level == LEVEL_SHIP) {
      (void)getship(&ship, Dir[Playernum - 1][Governor].shipno);
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
      strncpy(ship->class, buf, SHIP_NAMESIZE - 1);
      putship(ship);
      notify(Playernum, Governor, "Class set.\n");
      free(ship);
      return;
    } else {
      notify(Playernum, Governor,
             "You have to 'cs' to a factory to name the ship class.\n");
      return;
    }
  } else if (match(args[1], "block")) {
    /* name your alliance block */
    if (Governor) {
      notify(Playernum, Governor, "You are not authorized to do this.\n");
      return;
    }
    strncpy(Blocks[Playernum - 1].name, buf, RNAMESIZE - 1);
    Putblock(Blocks);
    notify(Playernum, Governor, "Done.\n");
  } else if (match(args[1], "star")) {
    if (Dir[Playernum - 1][Governor].level == LEVEL_STAR) {
      Race = races[Playernum - 1];
      if (!Race->God) {
        notify(Playernum, Governor, "Only dieties may name a star.\n");
        return;
      }
      strncpy(Stars[Dir[Playernum - 1][Governor].snum]->name, buf,
              NAMESIZE - 1);
      putstar(Stars[Dir[Playernum - 1][Governor].snum],
              Dir[Playernum - 1][Governor].snum);
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a star to name it.\n");
      return;
    }
  } else if (match(args[1], "planet")) {
    if (Dir[Playernum - 1][Governor].level == LEVEL_PLAN) {
      getstar(&Stars[Dir[Playernum - 1][Governor].snum],
              Dir[Playernum - 1][Governor].snum);
      Race = races[Playernum - 1];
      if (!Race->God) {
        notify(Playernum, Governor, "Only deity can rename planets.\n");
        return;
      }
      strncpy(Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum],
              buf, NAMESIZE - 1);
      putstar(Stars[Dir[Playernum - 1][Governor].snum],
              Dir[Playernum - 1][Governor].snum);
      deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum,
                0);
    } else {
      notify(Playernum, Governor, "You have to 'cs' to a planet to name it.\n");
      return;
    }
  } else if (match(args[1], "race")) {
    Race = races[Playernum - 1];
    if (Governor) {
      notify(Playernum, Governor, "You are not authorized to do this.\n");
      return;
    }
    strncpy(Race->name, buf, RNAMESIZE - 1);
    sprintf(buf, "Name changed to `%s'.\n", Race->name);
    notify(Playernum, Governor, buf);
    putrace(Race);
  } else if (match(args[1], "governor")) {
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

int MostAPs(int Playernum, startype *s) {
  register int i, t = 0;

  for (i = 0; i < MAXPLAYERS; i++)
    if (s->AP[i] >= t)
      t = s->AP[i];

  return (s->AP[Playernum - 1] == t);
}

void announce(int Playernum, int Governor, char *message, int mode) {
  racetype *Race;
  char symbol;

  Race = races[Playernum - 1];
  if (mode == SHOUT && !Race->God) {
    notify(Playernum, Governor,
           "You are not privileged to use this command.\n");
    return;
  }
  switch (Dir[Playernum - 1][Governor].level) {
  case LEVEL_UNIV:
    if (mode == ANN)
      mode = BROADCAST;
    break;
  default:
    if ((mode == ANN) &&
        !(!!isset(Stars[Dir[Playernum - 1][Governor].snum]->inhabited,
                  Playernum) ||
          Race->God)) {
      sprintf(buf,
              "You do not inhabit this system or have diety privileges.\n");
      notify(Playernum, Governor, buf);
      return;
    }
  }

  switch (mode) {
  case ANN:
    symbol = ':';
    break;
  case BROADCAST:
    symbol = '>';
    break;
  case SHOUT:
    symbol = '!';
    break;
  case THINK:
    symbol = '=';
    break;
  default:
    symbol = 0; // TODO(jeffbailey): Shouldn't happen.
  }
  sprintf(msg, "%s \"%s\" [%d,%d] %c %s\n", Race->name,
          Race->governor[Governor].name, Playernum, Governor, symbol, message);

  switch (mode) {
  case ANN:
    d_announce(Playernum, Governor, Dir[Playernum - 1][Governor].snum, msg);
    break;
  case BROADCAST:
    d_broadcast(Playernum, Governor, msg);
    break;
  case SHOUT:
    d_shout(Playernum, Governor, msg);
    break;
  case THINK:
    d_think(Playernum, Governor, msg);
    break;
  default:
    break;
  }
}
