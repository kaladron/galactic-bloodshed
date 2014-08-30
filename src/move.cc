// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  move.c -- move population and assault aliens on target sector */

#include "move.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GB_server.h"
#include "buffers.h"
#include "defense.h"
#include "files.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "load.h"
#include "mobiliz.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

static void mech_defend(int, int, int *, int, planettype *, int, int,
                        const sector &, int, int, const sector &);
static void mech_attack_people(shiptype *, int *, int *, racetype *, racetype *,
                               const sector &, int, int, int, char *, char *);
static void people_attack_mech(shiptype *, int, int, racetype *, racetype *,
                               const sector &, int, int, char *, char *);

void arm(int Playernum, int Governor, int APcount, int mode) {
  planettype *planet;
  racetype *Race;
  int x = -1, y = -1, max_allowed;
  int amount = 0;
  money_t cost = 0;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    notify(Playernum, Governor, "Change scope to planet level first.\n");
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  getplanet(&planet, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  if (planet->slaved_to > 0 && planet->slaved_to != Playernum) {
    notify(Playernum, Governor, "That planet has been enslaved!\n");
    free(planet);
    return;
  }

  sscanf(args[1], "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet->Maxx - 1 || y > planet->Maxy - 1) {
    notify(Playernum, Governor, "Illegal coordinates.\n");
    free(planet);
    return;
  }

  auto sect = getsector(*planet, x, y);
  if (sect.owner != Playernum) {
    notify(Playernum, Governor, "You don't own that sector.\n");
    free(planet);
    return;
  }
  if (mode) {
    max_allowed = MIN(sect.popn, planet->info[Playernum - 1].destruct *
                                     (sect.mobilization + 1));
    if (argn < 3)
      amount = max_allowed;
    else {
      sscanf(args[2], "%d", &amount);
      if (amount <= 0) {
        notify(Playernum, Governor,
               "You must specify a positive number of civs to arm.\n");
        free(planet);
        return;
      }
    }
    amount = MIN(amount, max_allowed);
    if (!amount) {
      notify(Playernum, Governor, "You can't arm any civilians now.\n");
      free(planet);
      return;
    }
    Race = races[Playernum - 1];
    /*    enlist_cost = ENLIST_TROOP_COST * amount; */
    money_t enlist_cost = Race->fighters * amount;
    if (enlist_cost > Race->governor[Governor].money) {
      sprintf(buf, "You need %ld money to enlist %d troops.\n", enlist_cost,
              amount);
      notify(Playernum, Governor, buf);
      free(planet);
      return;
    }
    Race->governor[Governor].money -= enlist_cost;
    putrace(Race);

    cost = MAX(1, amount / (sect.mobilization + 1));
    sect.troops += amount;
    sect.popn -= amount;
    planet->popn -= amount;
    planet->info[Playernum - 1].popn -= amount;
    planet->troops += amount;
    planet->info[Playernum - 1].troops += amount;
    planet->info[Playernum - 1].destruct -= cost;
    sprintf(buf,
            "%d population armed at a cost of %ldd (now %lu civilians, %lu "
            "military)\n",
            amount, cost, sect.popn, sect.troops);
    notify(Playernum, Governor, buf);
    sprintf(buf, "This mobilization cost %ld money.\n", enlist_cost);
    notify(Playernum, Governor, buf);
  } else {
    if (argn < 3)
      amount = sect.troops;
    else {
      sscanf(args[2], "%d", &amount);
      if (amount <= 0) {
        notify(Playernum, Governor,
               "You must specify a positive number of civs to arm.\n");
        free(planet);
        return;
      }
      amount = MIN(sect.troops, amount);
    }
    sect.popn += amount;
    sect.troops -= amount;
    planet->popn += amount;
    planet->troops -= amount;
    planet->info[Playernum - 1].popn += amount;
    planet->info[Playernum - 1].troops -= amount;
    sprintf(buf, "%d troops disarmed (now %lu civilians, %lu military)\n",
            amount, sect.popn, sect.troops);
    notify(Playernum, Governor, buf);
  }
  putsector(sect, *planet, x, y);
  putplanet(planet, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);
  free(planet);
}

void move_popn(int Playernum, int Governor, int what) {
  int Assault, APcost; /* unfriendly movement */
  int casualties, casualties2, casualties3;

  planettype *planet;
  int people, oldpopn, old2popn, old3popn, x = -1, y = -1, x2 = -1, y2 = -1;
  int old2owner, old2gov, absorbed, n, done;
  double astrength, dstrength;
  racetype *Race, *alien;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    sprintf(buf, "Wrong scope\n");
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  getplanet(&planet, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  if (planet->slaved_to > 0 && planet->slaved_to != Playernum) {
    sprintf(buf, "That planet has been enslaved!\n");
    notify(Playernum, Governor, buf);
    free(planet);
    return;
  }
  sscanf(args[1], "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet->Maxx - 1 || y > planet->Maxy - 1) {
    sprintf(buf, "Origin coordinates illegal.\n");
    notify(Playernum, Governor, buf);
    free(planet);
    return;
  }

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    auto sect = getsector(*planet, x, y);
    if (sect.owner != Playernum) {
      sprintf(buf, "You don't own sector %d,%d!\n", x, y);
      notify(Playernum, Governor, buf);
      free(planet);
      return;
    }
    if (!get_move(args[2][n++], x, y, &x2, &y2, planet)) {
      notify(Playernum, Governor, "Finished.\n");
      putplanet(planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      free(planet);
      return;
    }

    if (x2 < 0 || y2 < 0 || x2 > planet->Maxx - 1 || y2 > planet->Maxy - 1) {
      sprintf(buf, "Illegal coordinates %d,%d.\n", x2, y2);
      notify(Playernum, Governor, buf);
      putplanet(planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      free(planet);
      return;
    }

    if (!adjacent(x, y, x2, y2, planet)) {
      sprintf(buf, "Illegal move - to adjacent sectors only!\n");
      notify(Playernum, Governor, buf);
      free(planet);
      return;
    }

    /* ok, the move is legal */
    auto sect2 = getsector(*planet, x2, y2);
    if (argn >= 4) {
      sscanf(args[3], "%d", &people);
      if (people < 0) {
        if (what == CIV)
          people = sect.popn + people;
        else if (what == MIL)
          people = sect.troops + people;
      }
    } else {
      if (what == CIV)
        people = sect.popn;
      else if (what == MIL)
        people = sect.troops;
    }

    if ((what == CIV && (abs(people) > sect.popn)) ||
        (what == MIL && (abs(people) > sect.troops)) || people <= 0) {
      if (what == CIV)
        sprintf(buf, "Bad value - %lu civilians in [%d,%d]\n", sect.popn, x, y);
      else if (what == MIL)
        sprintf(buf, "Bad value - %lu troops in [%d,%d]\n", sect.troops, x, y);
      notify(Playernum, Governor, buf);
      putplanet(planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      free(planet);
      return;
    }

    sprintf(buf, "%d %s moved.\n", people,
            what == CIV ? "population" : "troops");
    notify(Playernum, Governor, buf);

    /* check for defending mechs */
    mech_defend(Playernum, Governor, &people, what, planet, x, y, sect, x2, y2,
                sect2);
    if (!people) {
      putsector(sect, *planet, x, y);
      putsector(sect2, *planet, x2, y2);
      putplanet(planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      free(planet);
      notify(Playernum, Governor, "Attack aborted.\n");
      return;
    }

    if (sect2.owner && (sect2.owner != Playernum))
      Assault = 1;
    else
      Assault = 0;

    /* action point cost depends on the size of the group being moved */
    if (what == CIV)
      APcost = MOVE_FACTOR * ((int)log(1.0 + (double)people) + Assault) + 1;
    else if (what == MIL)
      APcost = MOVE_FACTOR * ((int)log10(1.0 + (double)people) + Assault) + 1;

    if (!enufAP(Playernum, Governor,
                Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
                APcost)) {
      putplanet(planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      free(planet);
      return;
    }

    if (Assault) {
      ground_assaults[Playernum - 1][sect2.owner -
                                     1][Dir[Playernum - 1][Governor].snum] += 1;
      Race = races[Playernum - 1];
      alien = races[sect2.owner - 1];
      /* races find out about each other */
      alien->translate[Playernum - 1] =
          MIN(alien->translate[Playernum - 1] + 5, 100);
      Race->translate[sect2.owner - 1] =
          MIN(Race->translate[sect2.owner - 1] + 5, 100);

      old2owner = (int)(sect2.owner);
      old2gov =
          Stars[Dir[Playernum - 1][Governor].snum]->governor[sect2.owner - 1];
      if (what == CIV)
        sect.popn = MAX(0, sect.popn - people);
      else if (what == MIL)
        sect.troops = MAX(0, sect.troops - people);

      if (what == CIV)
        sprintf(buf, "%d civ assault %lu civ/%lu mil\n", people, sect2.popn,
                sect2.troops);
      else if (what == MIL)
        sprintf(buf, "%d mil assault %lu civ/%lu mil\n", people, sect2.popn,
                sect2.troops);
      notify(Playernum, Governor, buf);
      oldpopn = people;
      old2popn = sect2.popn;
      old3popn = sect2.troops;

      ground_attack(Race, alien, &people, what, &sect2.popn, &sect2.troops,
                    Defensedata[sect.condition], Defensedata[sect2.condition],
                    Race->likes[sect.condition], alien->likes[sect2.condition],
                    &astrength, &dstrength, &casualties, &casualties2,
                    &casualties3);

      sprintf(buf, "Attack: %.2f   Defense: %.2f.\n", astrength, dstrength);
      notify(Playernum, Governor, buf);

      if (!(sect2.popn + sect2.troops)) { /* we got 'em */
        sect2.owner = Playernum;
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (Race->absorb) {
          absorbed = int_rand(0, old2popn + old3popn);
          sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
          notify(Playernum, Governor, buf);
          sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n", absorbed);
          notify(old2owner, old2gov, buf);
        }
        if (what == CIV)
          sect2.popn = people + absorbed;
        else if (what == MIL) {
          sect2.popn = absorbed;
          sect2.troops = people;
        }
        adjust_morale(Race, alien, (int)alien->fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien->absorb) {
          absorbed = int_rand(0, oldpopn - people);
          sprintf(buf, "%d alien bodies absorbed.\n", absorbed);
          notify(old2owner, old2gov, buf);
          sprintf(buf, "Metamorphs have absorbed %d bodies!!!\n", absorbed);
          notify(Playernum, Governor, buf);
          sect2.popn += absorbed;
        }
        if (what == CIV)
          sect.popn += people;
        else if (what == MIL)
          sect.troops += people;
        adjust_morale(alien, Race, (int)Race->fighters);
      }

      sprintf(telegram_buf,
              "/%s/%s: %s [%d] %c(%d,%d) assaults %s [%d] %c(%d,%d) %s\n",
              Stars[Dir[Playernum - 1][Governor].snum]->name,
              Stars[Dir[Playernum - 1][Governor].snum]
                  ->pnames[Dir[Playernum - 1][Governor].pnum],
              Race->name, Playernum, Dessymbols[sect.condition], x, y,
              alien->name, alien->Playernum, Dessymbols[sect2.condition], x2,
              y2, (sect2.owner == Playernum ? "VICTORY" : "DEFEAT"));

      if (sect2.owner == Playernum) {
        sprintf(buf, "VICTORY! The sector is yours!\n");
        notify(Playernum, Governor, buf);
        sprintf(buf, "Sector CAPTURED!\n");
        strcat(telegram_buf, buf);
        if (people) {
          sprintf(buf, "%d %s move in.\n", people,
                  what == CIV ? "civilians" : "troops");
          notify(Playernum, Governor, buf);
        }
        planet->info[Playernum - 1].mob_points += (int)sect2.mobilization;
        planet->info[old2owner - 1].mob_points -= (int)sect2.mobilization;
      } else {
        sprintf(buf, "The invasion was repulsed; try again.\n");
        notify(Playernum, Governor, buf);
        sprintf(buf, "You fought them off!\n");
        strcat(telegram_buf, buf);
        done = 1; /* end loop */
      }

      if (!(sect.popn + sect.troops + people)) {
        sprintf(buf, "You killed all of them!\n");
        strcat(telegram_buf, buf);
        /* increase modifier */
        Race->translate[old2owner - 1] =
            MIN(Race->translate[old2owner - 1] + 5, 100);
      }
      if (!people) {
        sprintf(buf, "Oh no! They killed your party to the last man!\n");
        notify(Playernum, Governor, buf);
        /* increase modifier */
        alien->translate[Playernum - 1] =
            MIN(alien->translate[Playernum - 1] + 5, 100);
      }
      putrace(alien);
      putrace(Race);

      sprintf(buf, "Casualties: You: %d civ/%d mil, Them: %d %s\n", casualties2,
              casualties3, casualties, what == CIV ? "civ" : "mil");
      strcat(telegram_buf, buf);
      warn(old2owner, old2gov, telegram_buf);
      sprintf(buf, "Casualties: You: %d %s, Them: %d civ/%d mil\n", casualties,
              what == CIV ? "civ" : "mil", casualties2, casualties3);
      notify(Playernum, Governor, buf);
    } else {
      if (what == CIV) {
        sect.popn -= people;
        sect2.popn += people;
      } else if (what == MIL) {
        sect.troops -= people;
        sect2.troops += people;
      }
      if (!sect2.owner)
        planet->info[Playernum - 1].mob_points += (int)sect2.mobilization;
      sect2.owner = Playernum;
    }

    if (!(sect.popn + sect.troops)) {
      planet->info[Playernum - 1].mob_points -= (int)sect.mobilization;
      sect.owner = 0;
    }

    if (!(sect2.popn + sect2.troops)) {
      sect2.owner = 0;
      done = 1;
    }

    putsector(sect, *planet, x, y);
    putsector(sect2, *planet, x2, y2);

    deductAPs(Playernum, Governor, APcost, Dir[Playernum - 1][Governor].snum,
              0);
    x = x2;
    y = y2; /* get ready for the next round */
  }
  notify(Playernum, Governor, "Finished.\n");
  free(planet);
}

void walk(int Playernum, int Governor, int APcount) {
  shiptype *ship, *ship2, dummy;
  planettype *p;
  int shipno, x, y, i, sh, succ = 0, civ, mil;
  int oldowner, oldgov;
  int strength, strength1;
  racetype *Race, *alien;

  if (argn < 2) {
    notify(Playernum, Governor, "Walk what?\n");
    return;
  }
  sscanf(args[1] + (args[1][0] == '#'), "%d", &shipno);
  if (!getship(&ship, shipno)) {
    notify(Playernum, Governor, "No such ship.\n");
    return;
  }
  if (testship(Playernum, Governor, ship)) {
    notify(Playernum, Governor, "You do not control this ship.\n");
    free(ship);
    return;
  }
  if (ship->type != OTYPE_AFV) {
    notify(Playernum, Governor, "This ship doesn't walk!\n");
    free(ship);
    return;
  }
  if (!landed(ship)) {
    notify(Playernum, Governor, "This ship is not landed on a planet.\n");
    free(ship);
    return;
  }
  if (!ship->popn) {
    notify(Playernum, Governor, "No crew.\n");
    free(ship);
    return;
  }
  if (ship->fuel < AFV_FUEL_COST) {
    sprintf(buf, "You don't have %.1f fuel to move it.\n", AFV_FUEL_COST);
    notify(Playernum, Governor, buf);
    free(ship);
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[ship->storbits]->AP[Playernum - 1],
              APcount)) {
    free(ship);
    return;
  }
  getplanet(&p, (int)ship->storbits, (int)ship->pnumorbits);
  Race = races[Playernum - 1];

  if (!get_move(args[2][0], (int)ship->land_x, (int)ship->land_y, &x, &y, p)) {
    notify(Playernum, Governor, "Illegal move.\n");
    free(p);
    free(ship);
    return;
  }
  if (x < 0 || y < 0 || x > p->Maxx - 1 || y > p->Maxy - 1) {
    sprintf(buf, "Illegal coordinates %d,%d.\n", x, y);
    notify(Playernum, Governor, buf);
    free(ship);
    putplanet(p, Dir[Playernum - 1][Governor].snum,
              Dir[Playernum - 1][Governor].pnum);
    free(p);
    return;
  }
  /* check to see if player is permited on the sector type */
  auto sect = getsector(*p, x, y);
  if (!Race->likes[sect.condition]) {
    notify(Playernum, Governor,
           "Your ships cannot walk into that sector type!\n");
    free(ship);
    free(p);
    return;
  }
  /* if the sector is occupied by non-aligned AFVs, each one will attack */
  sh = p->ships;
  while (sh && ship->alive) {
    (void)getship(&ship2, sh);
    if (ship2->owner != Playernum && ship2->type == OTYPE_AFV &&
        landed(ship2) && retal_strength(ship2) && (ship2->land_x == x) &&
        (ship2->land_y == y)) {
      alien = races[ship2->owner - 1];
      if (!isset(Race->allied, (int)ship2->owner) ||
          !isset(alien->allied, Playernum)) {
        while ((strength = retal_strength(ship2)) &&
               (strength1 = retal_strength(ship))) {
          bcopy(ship, &dummy, sizeof(shiptype));
          use_destruct(ship2, strength);
          notify(Playernum, Governor, long_buf);
          warn((int)ship2->owner, (int)ship2->governor, long_buf);
          if (!ship2->alive) post(short_buf, COMBAT);
          notify_star(Playernum, Governor, (int)ship2->owner,
                      (int)ship->storbits, short_buf);
          if (strength1) {
            use_destruct(ship, strength1);
            notify(Playernum, Governor, long_buf);
            warn((int)ship2->owner, (int)ship2->governor, long_buf);
            if (!ship2->alive) post(short_buf, COMBAT);
            notify_star(Playernum, Governor, (int)ship2->owner,
                        (int)ship->storbits, short_buf);
          }
        }
        putship(ship2);
      }
    }
    sh = ship2->nextship;
    free(ship2);
  }
  /* if the sector is occupied by non-aligned player, attack them first */
  if (ship->popn && ship->alive && sect.owner && sect.owner != Playernum) {
    oldowner = sect.owner;
    oldgov = Stars[ship->storbits]->governor[sect.owner - 1];
    alien = races[oldowner - 1];
    if (!isset(Race->allied, oldowner) || !isset(alien->allied, Playernum)) {
      if (!retal_strength(ship)) {
        notify(Playernum, Governor, "You have nothing to attack with!\n");
        free(ship);
        free(p);
        return;
      }
      while ((sect.popn + sect.troops) && retal_strength(ship)) {
        civ = (int)sect.popn;
        mil = (int)sect.troops;
        mech_attack_people(ship, &civ, &mil, Race, alien, sect, x, y, 0,
                           long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien->Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, oldowner, (int)ship->storbits,
                    short_buf);
        post(short_buf, COMBAT);

        people_attack_mech(ship, sect.popn, sect.troops, alien, Race, sect, x,
                           y, long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien->Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, oldowner, (int)ship->storbits,
                    short_buf);
        if (!ship->alive) post(short_buf, COMBAT);

        sect.popn = civ;
        sect.troops = mil;
        if (!(sect.popn + sect.troops)) {
          p->info[sect.owner - 1].mob_points -= (int)sect.mobilization;
          sect.owner = 0;
        }
      }
    }
    putrace(alien);
    putrace(Race);
    putplanet(p, Dir[Playernum - 1][Governor].snum,
              Dir[Playernum - 1][Governor].pnum);
    putsector(sect, *p, x, y);
  }

  if ((sect.owner == Playernum || isset(Race->allied, (int)sect.owner) ||
       !sect.owner) &&
      ship->alive)
    succ = 1;

  if (ship->alive && ship->popn && succ) {
    sprintf(buf, "%s moving from %d,%d to %d,%d on %s.\n", Ship(ship),
            (int)ship->land_x, (int)ship->land_y, x, y, Dispshiploc(ship));
    ship->land_x = x;
    ship->land_y = y;
    use_fuel(ship, AFV_FUEL_COST);
    for (i = 1; i <= Num_races; i++)
      if (i != Playernum && p->info[i - 1].numsectsowned)
        notify(i,
               (int)Stars[Dir[Playernum - 1][Governor].snum]->governor[i - 1],
               buf);
  }
  putship(ship);
  deductAPs(Playernum, Governor, APcount, (int)ship->storbits, 0);
  free(ship);
  free(p);
}

int get_move(char direction, int x, int y, int *x2, int *y2,
             planettype *planet) {
  switch (direction) {
    case '1':
    case 'b':
      *x2 = x - 1;
      *y2 = y + 1;
      if (*x2 == -1) *x2 = planet->Maxx - 1;
      return 1;
    case '2':
    case 'k':
      *x2 = x;
      *y2 = y + 1;
      return 1;
    case '3':
    case 'n':
      *x2 = x + 1;
      *y2 = y + 1;
      if (*x2 == planet->Maxx) *x2 = 0;
      return 1;
    case '4':
    case 'h':
      *x2 = x - 1;
      *y2 = y;
      if (*x2 == -1) *x2 = planet->Maxx - 1;
      return 1;
    case '6':
    case 'l':
      *x2 = x + 1;
      *y2 = y;
      if (*x2 == planet->Maxx) *x2 = 0;
      return 1;
    case '7':
    case 'y':
      *x2 = x - 1;
      *y2 = y - 1;
      if (*x2 == -1) *x2 = planet->Maxx - 1;
      return 1;
    case '8':
    case 'j':
      *x2 = x;
      *y2 = y - 1;
      return 1;
    case '9':
    case 'u':
      *x2 = x + 1;
      *y2 = y - 1;
      if (*x2 == planet->Maxx) *x2 = 0;
      return 1;
    default:
      *x2 = x;
      *y2 = y;
      return 0;
  }
}

static void mech_defend(int Playernum, int Governor, int *people, int type,
                        planettype *p, int x, int y, const sector &s, int x2,
                        int y2, const sector &s2) {
  int sh;
  shiptype *ship;
  int civ = 0, mil = 0;
  int oldgov;
  racetype *Race, *alien;

  if (type == CIV)
    civ = *people;
  else
    mil = *people;

  sh = p->ships;
  Race = races[Playernum - 1];
  while (sh && (civ + mil)) {
    (void)getship(&ship, sh);
    if (ship->owner != Playernum && ship->type == OTYPE_AFV && landed(ship) &&
        retal_strength(ship) && (ship->land_x == x2) && (ship->land_y == y2)) {
      alien = races[ship->owner - 1];
      if (!isset(Race->allied, (int)ship->owner) ||
          !isset(alien->allied, Playernum)) {
        while ((civ + mil) && retal_strength(ship)) {
          oldgov = Stars[ship->storbits]->governor[alien->Playernum - 1];
          mech_attack_people(ship, &civ, &mil, alien, Race, s2, x2, y2, 1,
                             long_buf, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(alien->Playernum, oldgov, long_buf);
          if (civ + mil) {
            people_attack_mech(ship, civ, mil, Race, alien, s2, x2, y2,
                               long_buf, short_buf);
            notify(Playernum, Governor, long_buf);
            warn(alien->Playernum, oldgov, long_buf);
          }
        }
      }
      putship(ship);
    }
    sh = ship->nextship;
    free(ship);
  }
  *people = civ + mil;
}

static void mech_attack_people(shiptype *ship, int *civ, int *mil,
                               racetype *Race, racetype *alien,
                               const sector &sect, int x, int y, int ignore,
                               char *long_msg, char *short_msg) {
  int strength, oldciv, oldmil;
  double astrength, dstrength;
  int cas_civ, cas_mil, ammo;

  oldciv = *civ;
  oldmil = *mil;

  strength = retal_strength(ship);
  astrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (Race->likes[sect.condition] + 1.0) *
              morale_factor((double)(Race->morale - alien->morale));

  dstrength = (double)(10 * oldmil * alien->fighters + oldciv) * 0.01 *
              alien->tech * .01 * (alien->likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(alien->morale - Race->morale));

  if (ignore) {
    ammo = (int)log10((double)dstrength + 1.0) - 1;
    ammo = MIN(MAX(ammo, 0), strength);
    use_destruct(ship, ammo);
  } else
    use_destruct(ship, strength);

  cas_civ = int_rand(0, round_rand((double)oldciv * astrength / dstrength));
  cas_civ = MIN(oldciv, cas_civ);
  cas_mil = int_rand(0, round_rand((double)oldmil * astrength / dstrength));
  cas_mil = MIN(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  sprintf(short_msg, "%s: %s %s %s [%d]\n", Dispshiploc(ship), Ship(ship),
          (*civ + *mil) ? "attacked" : "slaughtered", alien->name,
          alien->Playernum);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\tBattle at %d,%d %s: %d guns fired on %d civ/%d mil\n", x, y,
          Desnames[sect.condition], strength, oldciv, oldmil);
  strcat(long_msg, buf);
  sprintf(buf, "\tAttack: %.3f   Defense: %.3f.\n", astrength, dstrength);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d civ/%d mil killed.\n", cas_civ, cas_mil);
  strcat(long_msg, buf);
}

static void people_attack_mech(shiptype *ship, int civ, int mil, racetype *Race,
                               racetype *alien, const sector &sect, int x,
                               int y, char *long_msg, char *short_msg) {
  int strength;
  double astrength, dstrength;
  int cas_civ, cas_mil, pdam, sdam, damage;
  int ammo;

  strength = retal_strength(ship);

  dstrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (alien->likes[sect.condition] + 1.0) *
              morale_factor((double)(alien->morale - Race->morale));

  astrength = (double)(10 * mil * Race->fighters + civ) * .01 * Race->tech *
              .01 * (Race->likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(Race->morale - alien->morale));
  ammo = (int)log10((double)astrength + 1.0) - 1;
  ammo = MIN(strength, MAX(0, ammo));
  use_destruct(ship, ammo);
  damage = int_rand(0, round_rand(100.0 * astrength / dstrength));
  damage = MIN(100, damage);
  ship->damage += damage;
  if (ship->damage >= 100) {
    ship->damage = 100;
    kill_ship(Race->Playernum, ship);
  }
  do_collateral(ship, damage, &cas_civ, &cas_mil, &pdam, &sdam);
  sprintf(short_msg, "%s: %s [%d] %s %s\n", Dispshiploc(ship), Race->name,
          Race->Playernum, ship->alive ? "attacked" : "DESTROYED", Ship(ship));
  strcpy(long_msg, short_msg);
  sprintf(buf, "\tBattle at %d,%d %s: %d civ/%d mil assault %s\n", x, y,
          Desnames[sect.condition], civ, mil, Shipnames[ship->type]);
  strcat(long_msg, buf);
  sprintf(buf, "\tAttack: %.3f   Defense: %.3f.\n", astrength, dstrength);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d%% damage inflicted for a total of %d%%\n", damage,
          ship->damage);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d civ/%d mil killed   %d prim/%d sec guns knocked out\n",
          cas_civ, cas_mil, pdam, sdam);
  strcat(long_msg, buf);
}

void ground_attack(racetype *Race, racetype *alien, int *people, int what,
                   population_t *civ, population_t *mil, unsigned int def1,
                   unsigned int def2, double alikes, double dlikes,
                   double *astrength, double *dstrength, int *casualties,
                   int *casualties2, int *casualties3) {
  int casualty_scale;

  *astrength = (double)(*people * Race->fighters * (what == MIL ? 10 : 1)) *
               (alikes + 1.0) * ((double)def1 + 1.0) *
               morale_factor((double)(Race->morale - alien->morale));
  *dstrength = (double)((*civ + *mil * 10) * alien->fighters) * (dlikes + 1.0) *
               ((double)def2 + 1.0) *
               morale_factor((double)(alien->morale - Race->morale));
  /* nuke both populations */
  casualty_scale = MIN(*people * (what == MIL ? 10 : 1) * Race->fighters,
                       (*civ + *mil * 10) * alien->fighters);

  *casualties = int_rand(
      0, round_rand((double)((casualty_scale / (what == MIL ? 10 : 1)) *
                             *dstrength / *astrength)));
  *casualties = MIN(*people, *casualties);
  *people -= *casualties;

  *casualties2 =
      int_rand(0, round_rand((double)casualty_scale * *astrength / *dstrength));
  *casualties2 = MIN(*civ, *casualties2);
  *civ -= *casualties2;
  /* and for troops */
  *casualties3 = int_rand(
      0, round_rand((double)(casualty_scale / 10) * *astrength / *dstrength));
  *casualties3 = MIN(*mil, *casualties3);
  *mil -= *casualties3;
}
