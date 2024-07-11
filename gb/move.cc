// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  move.c -- move population and assault aliens on target sector */

import gblib;
import std.compat;

#include "gb/move.h"

#include <strings.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/defense.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/load.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/shootblast.h"
#include "gb/star.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static void mech_defend(player_t, governor_t, int *, int, const Planet &, int,
                        int, const Sector &);
static void mech_attack_people(Ship *, int *, int *, Race &, Race &,
                               const Sector &, int, int, int, char *, char *);
static void people_attack_mech(Ship *, int, int, Race &, Race &, const Sector &,
                               int, int, char *, char *);

void arm(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int mode;
  if (argv[0] == "arm") {
    mode = 1;
  } else {
    mode = 0;  // disarm
  }
  int x = -1;
  int y = -1;
  int max_allowed;
  int amount = 0;
  money_t cost = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Change scope to planet level first.\n";
    return;
  }
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet = getplanet(g.snum, g.pnum);

  if (planet.slaved_to > 0 && planet.slaved_to != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return;
  }

  sscanf(argv[1].c_str(), "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet.Maxx - 1 || y > planet.Maxy - 1) {
    g.out << "Illegal coordinates.\n";
    return;
  }

  auto sect = getsector(planet, x, y);
  if (sect.owner != Playernum) {
    g.out << "You don't own that sector.\n";
    return;
  }
  if (mode) {
    max_allowed = MIN(sect.popn, planet.info[Playernum - 1].destruct *
                                     (sect.mobilization + 1));
    if (argv.size() < 3)
      amount = max_allowed;
    else {
      amount = std::stoul(argv[2]);
      if (amount <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return;
      }
    }
    amount = std::min(amount, max_allowed);
    if (!amount) {
      g.out << "You can't arm any civilians now.\n";
      return;
    }
    auto &race = races[Playernum - 1];
    /*    enlist_cost = ENLIST_TROOP_COST * amount; */
    money_t enlist_cost = race.fighters * amount;
    if (enlist_cost > race.governor[Governor].money) {
      sprintf(buf, "You need %ld money to enlist %d troops.\n", enlist_cost,
              amount);
      notify(Playernum, Governor, buf);
      return;
    }
    race.governor[Governor].money -= enlist_cost;
    putrace(race);

    cost = std::max(1U, amount / (sect.mobilization + 1));
    sect.troops += amount;
    sect.popn -= amount;
    planet.popn -= amount;
    planet.info[Playernum - 1].popn -= amount;
    planet.troops += amount;
    planet.info[Playernum - 1].troops += amount;
    planet.info[Playernum - 1].destruct -= cost;
    sprintf(buf,
            "%d population armed at a cost of %ldd (now %lu civilians, %lu "
            "military)\n",
            amount, cost, sect.popn, sect.troops);
    notify(Playernum, Governor, buf);
    sprintf(buf, "This mobilization cost %ld money.\n", enlist_cost);
    notify(Playernum, Governor, buf);
  } else {
    if (argv.size() < 3)
      amount = sect.troops;
    else {
      amount = std::stoi(argv[2]);
      if (amount <= 0) {
        g.out << "You must specify a positive number of civs to arm.\n";
        return;
      }
      amount = MIN(sect.troops, amount);
    }
    sect.popn += amount;
    sect.troops -= amount;
    planet.popn += amount;
    planet.troops -= amount;
    planet.info[Playernum - 1].popn += amount;
    planet.info[Playernum - 1].troops -= amount;
    sprintf(buf, "%d troops disarmed (now %lu civilians, %lu military)\n",
            amount, sect.popn, sect.troops);
    notify(Playernum, Governor, buf);
  }
  putsector(sect, planet, x, y);
  putplanet(planet, stars[g.snum], g.pnum);
}

void move_popn(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int what;
  if (argv[0] == "move") {
    what = CIV;
  } else {
    what = MIL;  // deploy
  }
  int Assault;
  int APcost; /* unfriendly movement */
  int casualties;
  int casualties2;
  int casualties3;

  int people;
  int oldpopn;
  int old2popn;
  int old3popn;
  int x = -1;
  int y = -1;
  int x2 = -1;
  int y2 = -1;
  int old2owner;
  int old2gov;
  int absorbed;
  int n;
  int done;
  double astrength;
  double dstrength;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "Wrong scope\n");
    return;
  }
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet = getplanet(g.snum, g.pnum);

  if (planet.slaved_to > 0 && planet.slaved_to != Playernum) {
    sprintf(buf, "That planet has been enslaved!\n");
    notify(Playernum, Governor, buf);
    return;
  }
  sscanf(argv[1].c_str(), "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet.Maxx - 1 || y > planet.Maxy - 1) {
    sprintf(buf, "Origin coordinates illegal.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    auto sect = getsector(planet, x, y);
    if (sect.owner != Playernum) {
      sprintf(buf, "You don't own sector %d,%d!\n", x, y);
      notify(Playernum, Governor, buf);
      return;
    }
    if (!get_move(argv[2][n++], x, y, &x2, &y2, planet)) {
      g.out << "Finished.\n";
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (x2 < 0 || y2 < 0 || x2 > planet.Maxx - 1 || y2 > planet.Maxy - 1) {
      sprintf(buf, "Illegal coordinates %d,%d.\n", x2, y2);
      notify(Playernum, Governor, buf);
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (!adjacent(x, y, x2, y2, planet)) {
      sprintf(buf, "Illegal move - to adjacent sectors only!\n");
      notify(Playernum, Governor, buf);
      return;
    }

    /* ok, the move is legal */
    auto sect2 = getsector(planet, x2, y2);
    if (argv.size() >= 4) {
      people = std::stoi(argv[3]);
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
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    sprintf(buf, "%d %s moved.\n", people,
            what == CIV ? "population" : "troops");
    notify(Playernum, Governor, buf);

    /* check for defending mechs */
    mech_defend(Playernum, Governor, &people, what, planet, x2, y2, sect2);
    if (!people) {
      putsector(sect, planet, x, y);
      putsector(sect2, planet, x2, y2);
      putplanet(planet, stars[g.snum], g.pnum);
      g.out << "Attack aborted.\n";
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

    if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcost)) {
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (Assault) {
      ground_assaults[Playernum - 1][sect2.owner - 1][g.snum] += 1;
      auto &race = races[Playernum - 1];
      auto &alien = races[sect2.owner - 1];
      /* races find out about each other */
      alien.translate[Playernum - 1] =
          MIN(alien.translate[Playernum - 1] + 5, 100);
      race.translate[sect2.owner - 1] =
          MIN(race.translate[sect2.owner - 1] + 5, 100);

      old2owner = (int)(sect2.owner);
      old2gov = stars[g.snum].governor[sect2.owner - 1];
      if (what == CIV)
        sect.popn = std::max(0L, sect.popn - people);
      else if (what == MIL)
        sect.troops = std::max(0L, sect.troops - people);

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

      ground_attack(race, alien, &people, what, &sect2.popn, &sect2.troops,
                    Defensedata[sect.condition], Defensedata[sect2.condition],
                    race.likes[sect.condition], alien.likes[sect2.condition],
                    &astrength, &dstrength, &casualties, &casualties2,
                    &casualties3);

      sprintf(buf, "Attack: %.2f   Defense: %.2f.\n", astrength, dstrength);
      notify(Playernum, Governor, buf);

      if (!(sect2.popn + sect2.troops)) { /* we got 'em */
        sect2.owner = Playernum;
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (race.absorb) {
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
        adjust_morale(race, alien, (int)alien.fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien.absorb) {
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
        adjust_morale(alien, race, (int)race.fighters);
      }

      sprintf(telegram_buf,
              "/%s/%s: %s [%d] %c(%d,%d) assaults %s [%d] %c(%d,%d) %s\n",
              stars[g.snum].name, stars[g.snum].pnames[g.pnum], race.name,
              Playernum, Dessymbols[sect.condition], x, y, alien.name,
              alien.Playernum, Dessymbols[sect2.condition], x2, y2,
              (sect2.owner == Playernum ? "VICTORY" : "DEFEAT"));

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
        planet.info[Playernum - 1].mob_points += (int)sect2.mobilization;
        planet.info[old2owner - 1].mob_points -= (int)sect2.mobilization;
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
        race.translate[old2owner - 1] =
            MIN(race.translate[old2owner - 1] + 5, 100);
      }
      if (!people) {
        sprintf(buf, "Oh no! They killed your party to the last man!\n");
        notify(Playernum, Governor, buf);
        /* increase modifier */
        alien.translate[Playernum - 1] =
            MIN(alien.translate[Playernum - 1] + 5, 100);
      }
      putrace(alien);
      putrace(race);

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
        planet.info[Playernum - 1].mob_points += (int)sect2.mobilization;
      sect2.owner = Playernum;
    }

    if (!(sect.popn + sect.troops)) {
      planet.info[Playernum - 1].mob_points -= (int)sect.mobilization;
      sect.owner = 0;
    }

    if (!(sect2.popn + sect2.troops)) {
      sect2.owner = 0;
      done = 1;
    }

    putsector(sect, planet, x, y);
    putsector(sect2, planet, x2, y2);

    deductAPs(g, APcost, g.snum);
    x = x2;
    y = y2; /* get ready for the next round */
  }
  g.out << "Finished.\n";
}

void walk(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const ap_t APcount = 1;
  Ship *ship;
  Ship dummy;
  int x;
  int y;
  int i;
  int succ = 0;
  int civ;
  int mil;
  player_t oldowner;
  governor_t oldgov;
  int strength;
  int strength1;

  if (argv.size() < 2) {
    g.out << "Walk what?\n";
    return;
  }
  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || !getship(&ship, *shipno)) {
    g.out << "No such ship.\n";
    return;
  }
  if (testship(*ship, Playernum, Governor)) {
    g.out << "You do not control this ship.\n";
    free(ship);
    return;
  }
  if (ship->type != ShipType::OTYPE_AFV) {
    g.out << "This ship doesn't walk!\n";
    free(ship);
    return;
  }
  if (!landed(*ship)) {
    g.out << "This ship is not landed on a planet.\n";
    free(ship);
    return;
  }
  if (!ship->popn) {
    g.out << "No crew.\n";
    free(ship);
    return;
  }
  if (ship->fuel < AFV_FUEL_COST) {
    sprintf(buf, "You don't have %.1f fuel to move it.\n", AFV_FUEL_COST);
    notify(Playernum, Governor, buf);
    free(ship);
    return;
  }
  if (!enufAP(Playernum, Governor, stars[ship->storbits].AP[Playernum - 1],
              APcount)) {
    free(ship);
    return;
  }
  auto p = getplanet((int)ship->storbits, (int)ship->pnumorbits);
  auto &race = races[Playernum - 1];

  if (!get_move(argv[2][0], (int)ship->land_x, (int)ship->land_y, &x, &y, p)) {
    g.out << "Illegal move.\n";
    free(ship);
    return;
  }
  if (x < 0 || y < 0 || x > p.Maxx - 1 || y > p.Maxy - 1) {
    sprintf(buf, "Illegal coordinates %d,%d.\n", x, y);
    notify(Playernum, Governor, buf);
    free(ship);
    putplanet(p, stars[g.snum], g.pnum);
    return;
  }
  /* check to see if player is permited on the sector type */
  auto sect = getsector(p, x, y);
  if (!race.likes[sect.condition]) {
    notify(Playernum, Governor,
           "Your ships cannot walk into that sector type!\n");
    free(ship);
    return;
  }
  /* if the sector is occupied by non-aligned AFVs, each one will attack */
  Shiplist shiplist{p.ships};
  for (auto ship2 : shiplist) {
    if (ship2.owner != Playernum && ship2.type == ShipType::OTYPE_AFV &&
        landed(ship2) && retal_strength(&ship2) && (ship2.land_x == x) &&
        (ship2.land_y == y)) {
      auto &alien = races[ship2.owner - 1];
      if (!isset(race.allied, ship2.owner) || !isset(alien.allied, Playernum)) {
        while ((strength = retal_strength(&ship2)) &&
               (strength1 = retal_strength(ship))) {
          bcopy(ship, &dummy, sizeof(Ship));
          use_destruct(ship2, strength);
          notify(Playernum, Governor, long_buf);
          warn(ship2.owner, ship2.governor, long_buf);
          if (!ship2.alive) post(short_buf, COMBAT);
          notify_star(Playernum, Governor, ship->storbits, short_buf);
          if (strength1) {
            use_destruct(*ship, strength1);
            notify(Playernum, Governor, long_buf);
            warn(ship2.owner, ship2.governor, long_buf);
            if (!ship2.alive) post(short_buf, COMBAT);
            notify_star(Playernum, Governor, ship->storbits, short_buf);
          }
        }
        putship(&ship2);
      }
    }
    if (!ship->alive) break;
  }
  /* if the sector is occupied by non-aligned player, attack them first */
  if (ship->popn && ship->alive && sect.owner && sect.owner != Playernum) {
    oldowner = sect.owner;
    oldgov = stars[ship->storbits].governor[sect.owner - 1];
    auto &alien = races[oldowner - 1];
    if (!isset(race.allied, oldowner) || !isset(alien.allied, Playernum)) {
      if (!retal_strength(ship)) {
        g.out << "You have nothing to attack with!\n";
        free(ship);
        return;
      }
      while ((sect.popn + sect.troops) && retal_strength(ship)) {
        civ = (int)sect.popn;
        mil = (int)sect.troops;
        mech_attack_people(ship, &civ, &mil, race, alien, sect, x, y, 0,
                           long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien.Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, ship->storbits, short_buf);
        post(short_buf, COMBAT);

        people_attack_mech(ship, sect.popn, sect.troops, alien, race, sect, x,
                           y, long_buf, short_buf);
        notify(Playernum, Governor, long_buf);
        warn(alien.Playernum, oldgov, long_buf);
        notify_star(Playernum, Governor, ship->storbits, short_buf);
        if (!ship->alive) post(short_buf, COMBAT);

        sect.popn = civ;
        sect.troops = mil;
        if (!(sect.popn + sect.troops)) {
          p.info[sect.owner - 1].mob_points -= (int)sect.mobilization;
          sect.owner = 0;
        }
      }
    }
    putrace(alien);
    putrace(race);
    putplanet(p, stars[g.snum], g.pnum);
    putsector(sect, p, x, y);
  }

  if ((sect.owner == Playernum || isset(race.allied, sect.owner) ||
       !sect.owner) &&
      ship->alive)
    succ = 1;

  if (ship->alive && ship->popn && succ) {
    sprintf(buf, "%s moving from %d,%d to %d,%d on %s.\n",
            ship_to_string(*ship).c_str(), (int)ship->land_x, (int)ship->land_y,
            x, y, Dispshiploc(ship));
    ship->land_x = x;
    ship->land_y = y;
    use_fuel(*ship, AFV_FUEL_COST);
    for (i = 1; i <= Num_races; i++)
      if (i != Playernum && p.info[i - 1].numsectsowned)
        notify(i, stars[g.snum].governor[i - 1], buf);
  }
  putship(ship);
  deductAPs(g, APcount, ship->storbits);
  free(ship);
}

int get_move(char direction, int x, int y, int *x2, int *y2,
             const Planet &planet) {
  switch (direction) {
    case '1':
    case 'b':
      *x2 = x - 1;
      *y2 = y + 1;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
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
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    case '4':
    case 'h':
      *x2 = x - 1;
      *y2 = y;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
      return 1;
    case '6':
    case 'l':
      *x2 = x + 1;
      *y2 = y;
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    case '7':
    case 'y':
      *x2 = x - 1;
      *y2 = y - 1;
      if (*x2 == -1) *x2 = planet.Maxx - 1;
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
      if (*x2 == planet.Maxx) *x2 = 0;
      return 1;
    default:
      *x2 = x;
      *y2 = y;
      return 0;
  }
}

static void mech_defend(player_t Playernum, governor_t Governor, int *people,
                        int type, const Planet &p, int x2, int y2,
                        const Sector &s2) {
  int civ = 0;
  int mil = 0;
  int oldgov;

  if (type == CIV)
    civ = *people;
  else
    mil = *people;

  auto &race = races[Playernum - 1];

  Shiplist shiplist{p.ships};
  for (auto ship : shiplist) {
    if (civ + mil == 0) break;
    if (ship.owner != Playernum && ship.type == ShipType::OTYPE_AFV &&
        landed(ship) && retal_strength(&ship) && (ship.land_x == x2) &&
        (ship.land_y == y2)) {
      auto &alien = races[ship.owner - 1];
      if (!isset(race.allied, ship.owner) || !isset(alien.allied, Playernum)) {
        while ((civ + mil) > 0 && retal_strength(&ship)) {
          oldgov = stars[ship.storbits].governor[alien.Playernum - 1];
          mech_attack_people(&ship, &civ, &mil, alien, race, s2, x2, y2, 1,
                             long_buf, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(alien.Playernum, oldgov, long_buf);
          if (civ + mil) {
            people_attack_mech(&ship, civ, mil, race, alien, s2, x2, y2,
                               long_buf, short_buf);
            notify(Playernum, Governor, long_buf);
            warn(alien.Playernum, oldgov, long_buf);
          }
        }
      }
      putship(&ship);
    }
  }
  *people = civ + mil;
}

static void mech_attack_people(Ship *ship, int *civ, int *mil, Race &race,
                               Race &alien, const Sector &sect, int x, int y,
                               int ignore, char *long_msg, char *short_msg) {
  int strength;
  int oldciv;
  int oldmil;
  double astrength;
  double dstrength;
  int cas_civ;
  int cas_mil;
  int ammo;

  oldciv = *civ;
  oldmil = *mil;

  strength = retal_strength(ship);
  astrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (race.likes[sect.condition] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));

  dstrength = (double)(10 * oldmil * alien.fighters + oldciv) * 0.01 *
              alien.tech * .01 * (alien.likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  if (ignore) {
    ammo = (int)log10((double)dstrength + 1.0) - 1;
    ammo = std::min(std::max(ammo, 0), strength);
    use_destruct(*ship, ammo);
  } else
    use_destruct(*ship, strength);

  cas_civ = int_rand(0, round_rand((double)oldciv * astrength / dstrength));
  cas_civ = MIN(oldciv, cas_civ);
  cas_mil = int_rand(0, round_rand((double)oldmil * astrength / dstrength));
  cas_mil = MIN(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  sprintf(short_msg, "%s: %s %s %s [%d]\n", Dispshiploc(ship),
          ship_to_string(*ship).c_str(),
          (*civ + *mil) ? "attacked" : "slaughtered", alien.name,
          alien.Playernum);
  strcpy(long_msg, short_msg);
  sprintf(buf, "\tBattle at %d,%d %s: %d guns fired on %d civ/%d mil\n", x, y,
          Desnames[sect.condition], strength, oldciv, oldmil);
  strcat(long_msg, buf);
  sprintf(buf, "\tAttack: %.3f   Defense: %.3f.\n", astrength, dstrength);
  strcat(long_msg, buf);
  sprintf(buf, "\t%d civ/%d mil killed.\n", cas_civ, cas_mil);
  strcat(long_msg, buf);
}

static void people_attack_mech(Ship *ship, int civ, int mil, Race &race,
                               Race &alien, const Sector &sect, int x, int y,
                               char *long_msg, char *short_msg) {
  int strength;
  double astrength;
  double dstrength;
  int cas_civ;
  int cas_mil;
  int pdam;
  int sdam;
  int damage;
  int ammo;

  strength = retal_strength(ship);

  dstrength = MECH_ATTACK * ship->tech * (double)strength *
              ((double)ship->armor + 1.0) * .01 *
              (100.0 - (double)ship->damage) * .01 *
              (alien.likes[sect.condition] + 1.0) *
              morale_factor((double)(alien.morale - race.morale));

  astrength = (double)(10 * mil * race.fighters + civ) * .01 * race.tech * .01 *
              (race.likes[sect.condition] + 1.0) *
              ((double)Defensedata[sect.condition] + 1.0) *
              morale_factor((double)(race.morale - alien.morale));
  ammo = (int)log10((double)astrength + 1.0) - 1;
  ammo = std::min(strength, std::max(0, ammo));
  use_destruct(*ship, ammo);
  damage = int_rand(0, round_rand(100.0 * astrength / dstrength));
  damage = std::min(100, damage);
  ship->damage += damage;
  if (ship->damage >= 100) {
    ship->damage = 100;
    kill_ship(race.Playernum, ship);
  }
  do_collateral(ship, damage, &cas_civ, &cas_mil, &pdam, &sdam);
  sprintf(short_msg, "%s: %s [%d] %s %s\n", Dispshiploc(ship), race.name,
          race.Playernum, ship->alive ? "attacked" : "DESTROYED",
          ship_to_string(*ship).c_str());
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

void ground_attack(Race &race, Race &alien, int *people, int what,
                   population_t *civ, population_t *mil, unsigned int def1,
                   unsigned int def2, double alikes, double dlikes,
                   double *astrength, double *dstrength, int *casualties,
                   int *casualties2, int *casualties3) {
  int casualty_scale;

  *astrength = (double)(*people * race.fighters * (what == MIL ? 10 : 1)) *
               (alikes + 1.0) * ((double)def1 + 1.0) *
               morale_factor((double)(race.morale - alien.morale));
  *dstrength = (double)((*civ + *mil * 10) * alien.fighters) * (dlikes + 1.0) *
               ((double)def2 + 1.0) *
               morale_factor((double)(alien.morale - race.morale));
  /* nuke both populations */
  casualty_scale = MIN(*people * (what == MIL ? 10 : 1) * race.fighters,
                       (*civ + *mil * 10) * alien.fighters);

  *casualties = int_rand(
      0, round_rand((double)((casualty_scale / (what == MIL ? 10 : 1)) *
                             *dstrength / *astrength)));
  *casualties = std::min(*people, *casualties);
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
