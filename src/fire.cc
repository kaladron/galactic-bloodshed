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

void fire(int Playernum, int Governor, int APcount, int cew) /* ship vs ship */
{
  shipnum_t fromship, toship, sh, nextshipno;
  shiptype *from, *to, *ship, dummy;
  planettype *p;
  int strength, maxstrength, retal, damage;

  sh = 0;  // TODO(jeffbailey): No idea what this is, init to 0.

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argn < 3) {
    notify(Playernum, Governor,
           "Syntax: 'fire <ship> <target> [<strength>]'.\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, args[1], from, &nextshipno) &&
        authorized(Governor, from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n", Ship(from));
        notify(Playernum, Governor, buf);
        free(from);
        continue;
      }
      if (from->whatorbits == LEVEL_UNIV) {
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
      sscanf(args[2] + (args[2][0] == '#'), "%lu", &toship);
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
          sprintf(buf, "%s isn't landed on a planet!\n", Ship(from));
          notify(Playernum, Governor, buf);
          free(from);
          free(to);
          continue;
        }
        if (!landed(to)) {
          sprintf(buf, "%s isn't landed on a planet!\n", Ship(from));
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
        getplanet(&p, (int)from->storbits, (int)from->pnumorbits);
        if (!adjacent((int)from->land_x, (int)from->land_y, (int)to->land_x,
                      (int)to->land_y, p)) {
          notify(Playernum, Governor, "You are not adjacent to your target!\n");
          free(from);
          free(to);
          free(p);
          continue;
        }
        free(p);
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

        if (argn >= 4)
          sscanf(args[3], "%d", &strength);
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
        if (to->whatorbits == LEVEL_STAR) /* star level ships */
          sh = Stars[to->storbits]->ships;
        if (to->whatorbits == LEVEL_PLAN) { /* planet level ships */
          getplanet(&p, (int)to->storbits, (int)to->pnumorbits);
          sh = p->ships;
          free(p);
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

void bombard(int Playernum, int Governor, int APcount) /* ship vs planet */
{
  shipnum_t fromship, nextshipno, sh;
  shiptype *from, *ship;
  planettype *p;
  int strength, maxstrength, x, y, ok, numdest, damage;
  int i;
  racetype *alien;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argn < 2) {
    notify(Playernum, Governor,
           "Syntax: 'bombard <ship> [<x,y> [<strength>]]'.\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, args[1], from, &nextshipno) &&
        authorized(Governor, from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n", Ship(from));
        notify(Playernum, Governor, buf);
        free(from);
        continue;
      }

      if (from->whatorbits != LEVEL_PLAN) {
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

      if (argn > 3)
        sscanf(args[3], "%d", &strength);
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
      getplanet(&p, (int)from->storbits, (int)from->pnumorbits);

      if (argn > 2) {
        sscanf(args[2], "%d,%d", &x, &y);
        if (x < 0 || x > p->Maxx - 1 || y < 0 || y > p->Maxy - 1) {
          notify(Playernum, Governor, "Illegal sector.\n");
          free(p);
          free(from);
          continue;
        }
      } else {
        x = int_rand(0, (int)p->Maxx - 1);
        y = int_rand(0, (int)p->Maxy - 1);
      }
      if (landed(from) &&
          !adjacent((int)from->land_x, (int)from->land_y, x, y, p)) {
        notify(Playernum, Governor, "You are not adjacent to that sector.\n");
        free(p);
        free(from);
        continue;
      }

      /* check to see if there are any planetary defense networks on the planet
       */
      ok = 1;
      sh = p->ships;
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
        free(p);
        free(from);
        continue;
      }

      numdest = shoot_ship_to_planet(from, p, strength, x, y, 1, 0, 0, long_buf,
                                     short_buf);

      if (numdest < 0) {
        notify(Playernum, Governor, "Illegal attack.\n");
        free(from);
        free(p);
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
          if (Nuked[i - 1] && !p->slaved_to) {
            /* add planet defense strength */
            alien = races[i - 1];
            strength = MIN(p->info[i - 1].destruct, p->info[i - 1].guns);

            p->info[i - 1].destruct -= strength;

            damage = shoot_planet_to_ship(alien, p, from, strength, long_buf,
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
        sh = p->ships;
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
      putplanet(p, (int)from->storbits, (int)from->pnumorbits);
      deductAPs(Playernum, Governor, APcount, (int)from->storbits, 0);

      free(from);
      free(p);
    } else
      free(from);
}

#ifdef DEFENSE
void defend(int Playernum, int Governor, int APcount) /* planet vs ship */
{
  int toship, sh;
  shiptype *to, *ship, dummy;
  planettype *p;
  int strength, retal, damage, x, y;
  int numdest;
  racetype *Race;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  /* get the planet from the players current scope */
  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    notify(Playernum, Governor, "You have to set scope to the planet first.\n");
    return;
  }

  if (argn < 3) {
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
  sscanf(args[1] + (args[1][0] == '#'), "%d", &toship);
  if (toship <= 0) {
    notify(Playernum, Governor, "Bad ship number.\n");
    return;
  }

  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  getplanet(&p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  if (!p->info[Playernum - 1].numsectsowned) {
    notify(Playernum, Governor, "You do not occupy any sectors here.\n");
    free(p);
    return;
  }

  if (p->slaved_to && p->slaved_to != Playernum) {
    notify(Playernum, Governor, "This planet is enslaved.\n");
    free(p);
    return;
  }

  if (!getship(&to, toship)) {
    free(p);
    return;
  }

  if (to->whatorbits != LEVEL_PLAN) {
    notify(Playernum, Governor, "The ship is not in planet orbit.\n");
    free(to);
    free(p);
    return;
  }

  if (to->storbits != Dir[Playernum - 1][Governor].snum ||
      to->pnumorbits != Dir[Playernum - 1][Governor].pnum) {
    notify(Playernum, Governor, "Target is not in orbit around this planet.\n");
    free(to);
    free(p);
    return;
  }

  if (landed(to)) {
    notify(Playernum, Governor, "Planet guns can't fire on landed ships.\n");
    free(to);
    free(p);
    return;
  }

  /* save defense strength for retaliation */
  check_retal_strength(to, &retal);
  bcopy(to, &dummy, sizeof(shiptype));

  sscanf(args[2], "%d,%d", &x, &y);

  if (x < 0 || x > p->Maxx - 1 || y < 0 || y > p->Maxy - 1) {
    notify(Playernum, Governor, "Illegal sector.\n");
    free(p);
    free(to);
    return;
  }

  /* check to see if you own the sector */
  auto sect = getsector(*p, x, y);
  if (sect->owner != Playernum) {
    notify(Playernum, Governor, "Nice try.\n");
    free(p);
    free(to);
    return;
  }

  if (argn >= 4)
    sscanf(args[3], "%d", &strength);
  else
    strength = p->info[Playernum - 1].guns;

  strength = MIN(strength, p->info[Playernum - 1].destruct);
  strength = MIN(strength, p->info[Playernum - 1].guns);

  if (strength <= 0) {
    sprintf(buf, "No attack - %d guns, %dd\n", p->info[Playernum - 1].guns,
            p->info[Playernum - 1].destruct);
    notify(Playernum, Governor, buf);
    free(p);
    free(to);
    return;
  }
  Race = races[Playernum - 1];

  damage = shoot_planet_to_ship(Race, p, to, strength, long_buf, short_buf);

  if (!to->alive && to->type == OTYPE_TOXWC) {
    /* get planet again since toxicity probably has changed */
    free(p);
    getplanet(&p, Dir[Playernum - 1][Governor].snum,
              Dir[Playernum - 1][Governor].pnum);
  }

  if (damage < 0) {
    sprintf(buf, "Target out of range  %d!\n", SYSTEMSIZE);
    notify(Playernum, Governor, buf);
    free(p);
    free(to);
    return;
  }

  p->info[Playernum - 1].destruct -= strength;
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

    if ((numdest = shoot_ship_to_planet(&dummy, p, strength, x, y, 1, 0, 0,
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
  }

  /* protecting ships retaliate individually if damage was inflicted */
  if (damage) {
    sh = p->ships;
    while (sh) {
      (void)getship(&ship, sh);
      if (ship->protect.on && (ship->protect.ship == toship) &&
          (ship->protect.ship == toship) && sh != toship && ship->alive &&
          ship->active) {
        if (laser_on(ship)) check_overload(ship, 0, &strength);
        check_retal_strength(ship, &strength);

        if ((numdest = shoot_ship_to_planet(ship, p, strength, x, y, 1, 0, 0,
                                            long_buf, short_buf)) >= 0) {
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
        putship(ship);
      }
      sh = ship->nextship;
      free(ship);
    }
  }

  /* write the ship stuff out to disk */
  putship(to);
  putplanet(p, Dir[Playernum - 1][Governor].snum,
            Dir[Playernum - 1][Governor].pnum);

  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  free(p);
  free(to);
  return;
}
#endif

void detonate(int Playernum, int Governor, int APcount) {
  shiptype *s;
  shipnum_t shipno, nextshipno;

  nextshipno = start_shiplist(Playernum, Governor, args[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, args[1], s, &nextshipno) &&
        authorized(Governor, s)) {
      if (s->type != STYPE_MINE) {
        notify(Playernum, Governor, "That is not a mine.\n");
        free(s);
        continue;
      } else if (!s->on) {
        notify(Playernum, Governor, "The mine is not activated.\n");
        free(s);
        continue;
      } else if (s->docked || s->whatorbits == LEVEL_SHIP) {
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

int adjacent(int fx, int fy, int tx, int ty, planettype *p) {
  if (abs(fy - ty) <= 1) {
    if (abs(fx - tx) <= 1)
      return 1;
    else if (fx == p->Maxx - 1 && tx == 0)
      return 1;
    else if (fx == 0 && tx == p->Maxx - 1)
      return 1;
    else
      return 0;
  } else
    return 0;
}

int landed(shiptype *ship) {
  return (ship->whatdest == LEVEL_PLAN && ship->docked);
}

static void check_overload(shiptype *ship, int cew, int *strength) {
  if ((ship->laser && ship->fire_laser) || cew) {
    if (int_rand(0, *strength) >
        (int)((1.0 - .01 * ship->damage) * ship->tech / 2.0)) {
      /* check to see if the ship blows up */
      sprintf(buf,
              "%s: Matter-antimatter EXPLOSION from overloaded crystal on %s\n",
              Dispshiploc(ship), Ship(ship));
      kill_ship((int)(ship->owner), ship);
      *strength = 0;
      warn((int)ship->owner, (int)ship->governor, buf);
      post(buf, COMBAT);
      notify_star((int)ship->owner, (int)ship->governor, 0, (int)ship->storbits,
                  buf);
    } else if (int_rand(0, *strength) >
               (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0)) {
      sprintf(buf, "%s: Crystal damaged from overloading on %s.\n",
              Dispshiploc(ship), Ship(ship));
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
