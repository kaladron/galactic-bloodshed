// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* fire.c -- fire at ship or planet from ship or planet */

#include "fire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GB_server.h"
#include "buffers.h"
#include "config.h"
#include "doship.h"
#include "files.h"
#include "files_shl.h"
#include "getplace.h"
#include "load.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

static void check_overload(shiptype *, int, int *);
static void check_retal_strength(shiptype *, int *);

/*! Ship vs ship */
void fire(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount;
  int cew;
  // This is called from dock.cc.
  if (argv[0] == "fire-from-dock") {
    // TODO(jeffbailey): It's not clear that cew is ever used as anything other
    // than a true/false value.
    cew = 3;
    APcount = 0;
  } else if (argv[0] == "cew") {
    cew = 1;
    APcount = 1;
  } else {  // argv[0] = fire
    cew = 0;
    APcount = 1;
  }
  shipnum_t fromship, toship, sh, nextshipno;
  shiptype *from, *to, *ship, dummy;
  int strength, maxstrength, retal, damage;

  sh = 0;  // TODO(jeffbailey): No idea what this is, init to 0.

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argv.size() < 3) {
    std::string msg =
        "Syntax: '" + argv[0] + " <ship> <target> [<strength>]'.\n";
    notify(Playernum, Governor, msg);
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, argv[1].c_str());
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, argv[1].c_str(), from, &nextshipno) &&
        authorized(Governor, from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n", Ship(*from).c_str());
        notify(Playernum, Governor, buf);
        free(from);
        continue;
      }
      if (from->whatorbits == ScopeLevel::LEVEL_UNIV) {
        if (!enufAP(Playernum, Governor, Sdata.AP[Playernum - 1], APcount)) {
          free(from);
          continue;
        }
      } else if (!enufAP(Playernum, Governor,
                         Stars[from->storbits]->AP[Playernum - 1], APcount)) {
        free(from);
        continue;
      }
      if (cew) {
        if (!from->cew) {
          notify(Playernum, Governor,
                 "That ship is not equipped to fire CEWs.\n");
          free(from);
          continue;
        }
        if (!from->mounted) {
          notify(Playernum, Governor,
                 "You need to have a crystal mounted to fire CEWs.\n");
          free(from);
          continue;
        }
      }
      sscanf(argv[2].c_str() + (argv[2].c_str()[0] == '#'), "%lu", &toship);
      if (toship <= 0) {
        notify(Playernum, Governor, "Bad ship number.\n");
        free(from);
        continue;
      }
      if (toship == fromship) {
        notify(Playernum, Governor, "Get real.\n");
        free(from);
        continue;
      }
      if (!getship(&to, toship)) {
        free(from);
        continue;
      }

      /* save defense attack strength for retaliation */
      check_retal_strength(to, &retal);
      bcopy(to, &dummy, sizeof(shiptype));

      if (from->type == OTYPE_AFV) {
        if (!landed(from)) {
          sprintf(buf, "%s isn't landed on a planet!\n", Ship(*from).c_str());
          notify(Playernum, Governor, buf);
          free(from);
          free(to);
          continue;
        }
        if (!landed(to)) {
          sprintf(buf, "%s isn't landed on a planet!\n", Ship(*from).c_str());
          notify(Playernum, Governor, buf);
          free(from);
          free(to);
          continue;
        }
      }
      if (landed(from) && landed(to)) {
        if ((from->storbits != to->storbits) ||
            (from->pnumorbits != to->pnumorbits)) {
          notify(Playernum, Governor,
                 "Landed ships can only attack other "
                 "landed ships if they are on the same "
                 "planet!\n");
          free(from);
          free(to);
          continue;
        }
        const auto &p = getplanet((int)from->storbits, (int)from->pnumorbits);
        if (!adjacent((int)from->land_x, (int)from->land_y, (int)to->land_x,
                      (int)to->land_y, p)) {
          notify(Playernum, Governor, "You are not adjacent to your target!\n");
          free(from);
          free(to);
          continue;
        }
      }
      if (cew) {
        if (from->fuel < (double)from->cew) {
          sprintf(buf, "You need %d fuel to fire CEWs.\n", from->cew);
          notify(Playernum, Governor, buf);
          free(from);
          free(to);
          continue;
        } else if (landed(from) || landed(to)) {
          notify(Playernum, Governor,
                 "CEWs cannot originate from or targeted "
                 "to ships landed on planets.\n");
          free(from);
          free(to);
          continue;
        } else {
          sprintf(buf, "CEW strength %d.\n", from->cew);
          notify(Playernum, Governor, buf);
          strength = from->cew / 2;
        }
      } else {
        check_retal_strength(from, &maxstrength);

        if (argv.size() >= 4)
          sscanf(argv[3].c_str(), "%d", &strength);
        else
          check_retal_strength(from, &strength);

        if (strength > maxstrength) {
          strength = maxstrength;
          sprintf(buf, "%s set to %d\n",
                  laser_on(from) ? "Laser strength" : "Guns", strength);
          notify(Playernum, Governor, buf);
        }
      }

      /* check to see if there is crystal overloads */
      if (laser_on(from) || cew) check_overload(from, cew, &strength);

      if (strength <= 0) {
        sprintf(buf, "No attack.\n");
        notify(Playernum, Governor, buf);
        putship(from);
        free(from);
        free(to);
        continue;
      }

      damage =
          shoot_ship_to_ship(from, to, strength, cew, 0, long_buf, short_buf);

      if (damage < 0) {
        notify(Playernum, Governor, "Illegal attack.\n");
        free(from);
        free(to);
        continue;
      }

      if (laser_on(from) || cew)
        use_fuel(from, 2.0 * (double)strength);
      else
        use_destruct(from, strength);

      if (!to->alive) post(short_buf, COMBAT);
      notify_star(Playernum, Governor, (int)to->owner, (int)from->storbits,
                  short_buf);
      warn((int)to->owner, (int)to->governor, long_buf);
      notify(Playernum, Governor, long_buf);
      /* defending ship retaliates */

      strength = 0;
      if (retal && damage && to->protect.self) {
        strength = retal;
        if (laser_on(to)) check_overload(to, 0, &strength);

        if ((damage = shoot_ship_to_ship(&dummy, from, strength, 0, 1, long_buf,
                                         short_buf)) >= 0) {
          if (laser_on(to))
            use_fuel(to, 2.0 * (double)strength);
          else
            use_destruct(to, strength);
          if (!from->alive) post(short_buf, COMBAT);
          notify_star(Playernum, Governor, (int)to->owner, (int)from->storbits,
                      short_buf);
          notify(Playernum, Governor, long_buf);
          warn((int)to->owner, (int)to->governor, long_buf);
        }
      }
      /* protecting ships retaliate individually if damage was inflicted */
      /* AFVs immune to retaliation of this type */
      if (damage && from->alive && from->type != OTYPE_AFV) {
        if (to->whatorbits == ScopeLevel::LEVEL_STAR) /* star level ships */
          sh = Stars[to->storbits]->ships;
        if (to->whatorbits == ScopeLevel::LEVEL_PLAN) { /* planet level ships */
          const auto &p = getplanet((int)to->storbits, (int)to->pnumorbits);
          sh = p.ships;
        }
        while (sh && from->alive) {
          (void)getship(&ship, sh);
          if (ship->protect.on && (ship->protect.ship == toship) &&
              (ship->protect.ship == toship) && sh != fromship &&
              sh != toship && ship->alive && ship->active) {
            check_retal_strength(ship, &strength);
            if (laser_on(ship)) check_overload(ship, 0, &strength);

            if ((damage = shoot_ship_to_ship(ship, from, strength, 0, 0,
                                             long_buf, short_buf)) >= 0) {
              if (laser_on(ship))
                use_fuel(ship, 2.0 * (double)strength);
              else
                use_destruct(ship, strength);
              if (!from->alive) post(short_buf, COMBAT);
              notify_star(Playernum, Governor, (int)ship->owner,
                          (int)from->storbits, short_buf);
              notify(Playernum, Governor, long_buf);
              warn((int)ship->owner, (int)ship->governor, long_buf);
            }
            putship(ship);
          }
          sh = ship->nextship;
          free(ship);
        }
      }
      putship(from);
      putship(to);
      deductAPs(Playernum, Governor, APcount, (int)from->storbits, 0);

      free(from);
      free(to);
    } else
      free(from);
}

/*! Ship vs planet */
void bombard(const command_t &argv, GameObj &g) {
  int Playernum = g.player;
  int Governor = g.governor;
  int APcount = 1;
  shipnum_t fromship, nextshipno, sh;
  shiptype *from, *ship;
  int strength, maxstrength, x, y, ok, numdest, damage;
  int i;
  racetype *alien;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argv.size() < 2) {
    notify(Playernum, Governor,
           "Syntax: 'bombard <ship> [<x,y> [<strength>]]'.\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, argv[1].c_str());
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, argv[1].c_str(), from, &nextshipno) &&
        authorized(Governor, from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n", Ship(*from).c_str());
        notify(Playernum, Governor, buf);
        free(from);
        continue;
      }

      if (from->whatorbits != ScopeLevel::LEVEL_PLAN) {
        notify(Playernum, Governor,
               "You must be in orbit around a planet to bombard.\n");
        free(from);
        continue;
      }
      if (from->type == OTYPE_AFV && !landed(from)) {
        notify(Playernum, Governor, "This ship is not landed on the planet.\n");
        free(from);
        continue;
      } else if (!enufAP(Playernum, Governor,
                         Stars[from->storbits]->AP[Playernum - 1], APcount)) {
        free(from);
        continue;
      }

      check_retal_strength(from, &maxstrength);

      if (argv.size() > 3)
        sscanf(argv[3].c_str(), "%d", &strength);
      else
        check_retal_strength(from, &strength);

      if (strength > maxstrength) {
        strength = maxstrength;
        sprintf(buf, "%s set to %d\n",
                laser_on(from) ? "Laser strength" : "Guns", strength);
        notify(Playernum, Governor, buf);
      }

      /* check to see if there is crystal overload */
      if (laser_on(from)) check_overload(from, 0, &strength);

      if (strength <= 0) {
        sprintf(buf, "No attack.\n");
        notify(Playernum, Governor, buf);
        putship(from);
        free(from);
        continue;
      }

      /* get planet */
      auto p = getplanet((int)from->storbits, (int)from->pnumorbits);

      if (argv.size() > 2) {
        sscanf(argv[2].c_str(), "%d,%d", &x, &y);
        if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
          notify(Playernum, Governor, "Illegal sector.\n");
          free(from);
          continue;
        }
      } else {
        x = int_rand(0, (int)p.Maxx - 1);
        y = int_rand(0, (int)p.Maxy - 1);
      }
      if (landed(from) &&
          !adjacent((int)from->land_x, (int)from->land_y, x, y, p)) {
        notify(Playernum, Governor, "You are not adjacent to that sector.\n");
        free(from);
        continue;
      }

      /* check to see if there are any planetary defense networks on the planet
       */
      ok = 1;
      sh = p.ships;
      while (sh && ok) {
        (void)getship(&ship, sh);
        ok = !(ship->alive && ship->type == OTYPE_PLANDEF &&
               ship->owner != Playernum);
        sh = ship->nextship;
        free(ship);
      }

      if (!ok && !landed(from)) {
        notify(Playernum, Governor,
               "Target has planetary defense "
               "networks.\nThese have to be eliminated "
               "before you can attack sectors.\n");
        free(from);
        continue;
      }

      auto smap = getsmap(p);
      numdest = shoot_ship_to_planet(from, &p, strength, x, y, smap, 0, 0,
                                     long_buf, short_buf);
      putsmap(smap, p);

      if (numdest < 0) {
        notify(Playernum, Governor, "Illegal attack.\n");
        free(from);
        continue;
      }

      if (laser_on(from))
        use_fuel(from, 2.0 * (double)strength);
      else
        use_destruct(from, strength);

      post(short_buf, COMBAT);
      notify_star(Playernum, Governor, 0, (int)from->storbits, short_buf);
      for (i = 1; i <= Num_races; i++)
        if (Nuked[i - 1])
          warn(i, Stars[from->storbits]->governor[i - 1], long_buf);
      notify(Playernum, Governor, long_buf);

#ifdef DEFENSE
      /* planet retaliates - AFVs are immune to this */
      if (numdest && from->type != OTYPE_AFV) {
        damage = 0;
        for (i = 1; i <= Num_races; i++)
          if (Nuked[i - 1] && !p.slaved_to) {
            /* add planet defense strength */
            alien = races[i - 1];
            strength = MIN(p.info[i - 1].destruct, p.info[i - 1].guns);

            p.info[i - 1].destruct -= strength;

            damage = shoot_planet_to_ship(alien, from, strength, long_buf,
                                          short_buf);
            warn(i, (int)Stars[from->storbits]->governor[i - 1], long_buf);
            notify(Playernum, Governor, long_buf);
            if (!from->alive) post(short_buf, COMBAT);
            notify_star(Playernum, Governor, i, (int)from->storbits, short_buf);
          }
      }
#endif

      /* protecting ships retaliate individually if damage was inflicted */
      /* AFVs are immune to this */
      if (numdest && from->alive && from->type != OTYPE_AFV) {
        sh = p.ships;
        while (sh && from->alive) {
          (void)getship(&ship, sh);

          if (ship->protect.planet && sh != fromship && ship->alive &&
              ship->active) {
            if (laser_on(ship)) check_overload(ship, 0, &strength);

            check_retal_strength(ship, &strength);

            if ((damage = shoot_ship_to_ship(ship, from, strength, 0, 0,
                                             long_buf, short_buf)) >= 0) {
              if (laser_on(ship))
                use_fuel(ship, 2.0 * (double)strength);
              else
                use_destruct(ship, strength);
              if (!from->alive) post(short_buf, COMBAT);
              notify_star(Playernum, Governor, (int)ship->owner,
                          (int)from->storbits, short_buf);
              warn((int)ship->owner, (int)ship->governor, long_buf);
              notify(Playernum, Governor, long_buf);
            }
            putship(ship);
          }
          sh = ship->nextship;
          free(ship);
        }
      }

      /* write the stuff to disk */
      putship(from);
      putplanet(p, Stars[from->storbits], (int)from->pnumorbits);
      deductAPs(Playernum, Governor, APcount, (int)from->storbits, 0);

      free(from);
    } else
      free(from);
}

#ifdef DEFENSE
/*! Planet vs ship */
void defend(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 1;
  int toship, sh;
  shiptype *to, *ship, dummy;
  int strength, retal, damage, x, y;
  int numdest;
  racetype *Race;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  /* get the planet from the players current scope */
  if (Dir[Playernum - 1][Governor].level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor, "You have to set scope to the planet first.\n");
    return;
  }

  if (argv.size() < 3) {
    notify(Playernum, Governor,
           "Syntax: 'defend <ship> <sector> [<strength>]'.\n");
    return;
  }
  if (Governor &&
      Stars[Dir[Playernum - 1][Governor].snum]->governor[Playernum - 1] !=
          Governor) {
    notify(Playernum, Governor,
           "You are not authorized to do that in this system.\n");
    return;
  }
  sscanf(argv[1].c_str() + (argv[1].c_str()[0] == '#'), "%d", &toship);
  if (toship <= 0) {
    notify(Playernum, Governor, "Bad ship number.\n");
    return;
  }

  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  auto p = getplanet(Dir[Playernum - 1][Governor].snum,
                     Dir[Playernum - 1][Governor].pnum);

  if (!p.info[Playernum - 1].numsectsowned) {
    notify(Playernum, Governor, "You do not occupy any sectors here.\n");
    return;
  }

  if (p.slaved_to && p.slaved_to != Playernum) {
    notify(Playernum, Governor, "This planet is enslaved.\n");
    return;
  }

  if (!getship(&to, toship)) {
    return;
  }

  if (to->whatorbits != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor, "The ship is not in planet orbit.\n");
    free(to);
    return;
  }

  if (to->storbits != Dir[Playernum - 1][Governor].snum ||
      to->pnumorbits != Dir[Playernum - 1][Governor].pnum) {
    notify(Playernum, Governor, "Target is not in orbit around this planet.\n");
    free(to);
    return;
  }

  if (landed(to)) {
    notify(Playernum, Governor, "Planet guns can't fire on landed ships.\n");
    free(to);
    return;
  }

  /* save defense strength for retaliation */
  check_retal_strength(to, &retal);
  bcopy(to, &dummy, sizeof(shiptype));

  sscanf(argv[2].c_str(), "%d,%d", &x, &y);

  if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
    notify(Playernum, Governor, "Illegal sector.\n");
    free(to);
    return;
  }

  /* check to see if you own the sector */
  auto sect = getsector(p, x, y);
  if (sect.owner != Playernum) {
    notify(Playernum, Governor, "Nice try.\n");
    free(to);
    return;
  }

  if (argv.size() >= 4)
    sscanf(argv[3].c_str(), "%d", &strength);
  else
    strength = p.info[Playernum - 1].guns;

  strength = MIN(strength, p.info[Playernum - 1].destruct);
  strength = MIN(strength, p.info[Playernum - 1].guns);

  if (strength <= 0) {
    sprintf(buf, "No attack - %d guns, %dd\n", p.info[Playernum - 1].guns,
            p.info[Playernum - 1].destruct);
    notify(Playernum, Governor, buf);
    free(to);
    return;
  }
  Race = races[Playernum - 1];

  damage = shoot_planet_to_ship(Race, to, strength, long_buf, short_buf);

  if (!to->alive && to->type == OTYPE_TOXWC) {
    /* get planet again since toxicity probably has changed */
    p = getplanet(Dir[Playernum - 1][Governor].snum,
                  Dir[Playernum - 1][Governor].pnum);
  }

  if (damage < 0) {
    sprintf(buf, "Target out of range  %d!\n", SYSTEMSIZE);
    notify(Playernum, Governor, buf);
    free(to);
    return;
  }

  p.info[Playernum - 1].destruct -= strength;
  if (!to->alive) post(short_buf, COMBAT);
  notify_star(Playernum, Governor, (int)to->owner, (int)to->storbits,
              short_buf);
  warn((int)to->owner, (int)to->governor, long_buf);
  notify(Playernum, Governor, long_buf);

  /* defending ship retaliates */

  strength = 0;
  if (retal && damage && to->protect.self) {
    strength = retal;
    if (laser_on(to)) check_overload(to, 0, &strength);

    auto smap = getsmap(p);
    if ((numdest = shoot_ship_to_planet(&dummy, &p, strength, x, y, smap, 0, 0,
                                        long_buf, short_buf)) >= 0) {
      if (laser_on(to))
        use_fuel(to, 2.0 * (double)strength);
      else
        use_destruct(to, strength);

      post(short_buf, COMBAT);
      notify_star(Playernum, Governor, (int)to->owner, (int)to->storbits,
                  short_buf);
      notify(Playernum, Governor, long_buf);
      warn((int)to->owner, (int)to->governor, long_buf);
    }
    putsmap(smap, p);
  }

  /* protecting ships retaliate individually if damage was inflicted */
  if (damage) {
    sh = p.ships;
    while (sh) {
      (void)getship(&ship, sh);
      if (ship->protect.on && (ship->protect.ship == toship) &&
          (ship->protect.ship == toship) && sh != toship && ship->alive &&
          ship->active) {
        if (laser_on(ship)) check_overload(ship, 0, &strength);
        check_retal_strength(ship, &strength);

        auto smap = getsmap(p);
        if ((numdest = shoot_ship_to_planet(ship, &p, strength, x, y, smap, 0,
                                            0, long_buf, short_buf)) >= 0) {
          if (laser_on(ship))
            use_fuel(ship, 2.0 * (double)strength);
          else
            use_destruct(ship, strength);
          post(short_buf, COMBAT);
          notify_star(Playernum, Governor, (int)ship->owner,
                      (int)ship->storbits, short_buf);
          notify(Playernum, Governor, long_buf);
          warn((int)ship->owner, (int)ship->governor, long_buf);
        }
        putsmap(smap, p);
        putship(ship);
      }
      sh = ship->nextship;
      free(ship);
    }
  }

  /* write the ship stuff out to disk */
  putship(to);
  putplanet(p, Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].pnum);

  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  free(to);
  return;
}
#endif

void detonate(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (argv.size() < 3) {
    std::string msg = "Syntax: '" + argv[0] + " <mine>'\n";
    notify(Playernum, Governor, msg);
    return;
  }

  shiptype *s;
  shipnum_t shipno, nextshipno;

  nextshipno = start_shiplist(Playernum, Governor, argv[1].c_str());

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1].c_str(), s, &nextshipno) &&
        authorized(Governor, s)) {
      if (s->type != STYPE_MINE) {
        notify(Playernum, Governor, "That is not a mine.\n");
        free(s);
        continue;
      } else if (!s->on) {
        notify(Playernum, Governor, "The mine is not activated.\n");
        free(s);
        continue;
      } else if (s->docked || s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        notify(Playernum, Governor, "The mine is docked or landed.\n");
        free(s);
        continue;
      }
      free(s);
      domine(shipno, 1);
    } else
      free(s);
}

int retal_strength(shiptype *s) {
  int strength = 0, avail = 0;

  if (!s->alive) return 0;
  if (!Shipdata[s->type][ABIL_SPEED] && !landed(s)) return 0;
  /* land based ships */
  if (!s->popn && (s->type != OTYPE_BERS)) return 0;

  if (s->guns == PRIMARY)
    avail = (s->type == STYPE_FIGHTER || s->type == OTYPE_AFV ||
             s->type == OTYPE_BERS)
                ? s->primary
                : MIN(s->popn, s->primary);
  else if (s->guns == SECONDARY)
    avail = (s->type == STYPE_FIGHTER || s->type == OTYPE_AFV ||
             s->type == OTYPE_BERS)
                ? s->secondary
                : MIN(s->popn, s->secondary);
  else
    avail = 0;

  avail = MIN(s->retaliate, avail);
  strength = MIN(s->destruct, avail);
  return strength;
}

int adjacent(int fx, int fy, int tx, int ty, const Planet &p) {
  if (abs(fy - ty) <= 1) {
    if (abs(fx - tx) <= 1)
      return 1;
    else if (fx == p.Maxx - 1 && tx == 0)
      return 1;
    else if (fx == 0 && tx == p.Maxx - 1)
      return 1;
    else
      return 0;
  } else
    return 0;
}

int landed(shiptype *ship) {
  return (ship->whatdest == ScopeLevel::LEVEL_PLAN && ship->docked);
}

static void check_overload(shiptype *ship, int cew, int *strength) {
  if ((ship->laser && ship->fire_laser) || cew) {
    if (int_rand(0, *strength) >
        (int)((1.0 - .01 * ship->damage) * ship->tech / 2.0)) {
      /* check to see if the ship blows up */
      sprintf(buf,
              "%s: Matter-antimatter EXPLOSION from overloaded crystal on %s\n",
              Dispshiploc(ship), Ship(*ship).c_str());
      kill_ship((int)(ship->owner), ship);
      *strength = 0;
      warn((int)ship->owner, (int)ship->governor, buf);
      post(buf, COMBAT);
      notify_star((int)ship->owner, (int)ship->governor, 0, (int)ship->storbits,
                  buf);
    } else if (int_rand(0, *strength) >
               (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0)) {
      sprintf(buf, "%s: Crystal damaged from overloading on %s.\n",
              Dispshiploc(ship), Ship(*ship).c_str());
      ship->fire_laser = 0;
      ship->mounted = 0;
      *strength = 0;
      warn((int)ship->owner, (int)ship->governor, buf);
    }
  }
}

static void check_retal_strength(shiptype *ship, int *strength) {
  *strength = 0;
  if (ship->active && ship->alive) { /* irradiated ships dont retaliate */
    if (laser_on(ship))
      *strength = MIN(ship->fire_laser, (int)ship->fuel / 2);
    else
      *strength = retal_strength(ship);
  }
}

int laser_on(shiptype *ship) { return (ship->laser && ship->fire_laser); }
