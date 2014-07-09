// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  land.c -- land a ship
 *  also.... dock -- dock a ship w/ another ship
 *  and..... assault -- a very un-PC version of land/dock
 */

#define EXTERN extern
#include "land.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "config.h"
#include "files.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "load.h"
#include "max.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

static int roll;

void land(int Playernum, int Governor, int APcount) {
  shiptype *s, *s2;
  planettype *p;
  sectortype *sect;

  int shipno, ship2no, x = -1, y = -1, i, numdest, strength;
  double fuel;
  double Dist;
  racetype *Race, *alien;
  int nextshipno;

  numdest = 0; // TODO(jeffbailey): Init to zero.

  if (argn < 2) {
    notify(Playernum, Governor, "Land what?\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, args[1], s, &nextshipno) &&
        authorized(Governor, s)) {
      if (overloaded(s)) {
        sprintf(buf, "%s is too overloaded to land.\n", Ship(s));
        notify(Playernum, Governor, buf);
        free(s);
        continue;
      }
      if (s->type == OTYPE_QUARRY) {
        notify(Playernum, Governor, "You can't load quarries onto ship.\n");
        free(s);
        continue;
      }
      if (docked(s)) {
        notify(Playernum, Governor, "That ship is docked to another ship.\n");
        free(s);
        continue;
      }
      if (args[2][0] == '#') {
        /* attempting to land on a friendly ship (for carriers/stations/etc) */
        sscanf(args[2] + 1, "%d", &ship2no);
        if (!getship(&s2, ship2no)) {
          sprintf(buf, "Ship #%d wasn't found.\n", ship2no);
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (testship(Playernum, Governor, s2)) {
          notify(Playernum, Governor, "Illegal format.\n");
          free(s);
          free(s2);
          continue;
        }
        if (s2->type == OTYPE_FACTORY) {
          notify(Playernum, Governor, "Can't land on factories.\n");
          free(s);
          free(s2);
          continue;
        }
        if (landed(s)) {
          if (!landed(s2)) {
            sprintf(buf, "%s is not landed on a planet.\n", Ship(s2));
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          if (s2->storbits != s->storbits) {
            notify(Playernum, Governor,
                   "These ships are not in the same star system.\n");
            free(s);
            free(s2);
            continue;
          }
          if (s2->pnumorbits != s->pnumorbits) {
            notify(Playernum, Governor,
                   "These ships are not landed on the same planet.\n");
            free(s);
            free(s2);
            continue;
          }
          if ((s2->land_x != s->land_x) || (s2->land_y != s->land_y)) {
            notify(Playernum, Governor,
                   "These ships are not in the same sector.\n");
            free(s);
            free(s2);
            continue;
          }
          if (s->on) {
            sprintf(buf, "%s must be turned off before loading.\n", Ship(s));
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          if (Size(s) > Hanger(s2)) {
            sprintf(buf, "Mothership does not have %d hanger space available "
                         "to load ship.\n",
                    Size(s));
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          /* ok, load 'em up */
          remove_sh_plan(s);
          /* get the target ship again because it had a pointer changed (and put
           * to disk) in the remove routines */
          free(s2);
          (void)getship(&s2, ship2no);
          insert_sh_ship(s, s2);
          /* increase mass of mothership */
          s2->mass += s->mass;
          s2->hanger += Size(s);
          fuel = 0.0;
          sprintf(buf, "%s loaded onto %s using %.1f fuel.\n", Ship(s),
                  Ship(s2), fuel);
          notify(Playernum, Governor, buf);
          s->docked = 1;
          putship(s2);
          free(s2);
        } else if (s->docked) {
          sprintf(buf, "%s is already docked or landed.\n", Ship(s));
          notify(Playernum, Governor, buf);
          free(s);
          free(s2);
          continue;
        } else {
          /* Check if the ships are in the same scope level. Maarten */
          if (s->whatorbits != s2->whatorbits) {
            notify(Playernum, Governor,
                   "Those ships are not in the same scope.\n");
            free(s);
            free(s2);
            continue;
          }

          /* check to see if close enough to land */
          Dist = sqrt((double)Distsq(s2->xpos, s2->ypos, s->xpos, s->ypos));
          if (Dist > DIST_TO_DOCK) {
            sprintf(buf, "%s must be %.2f or closer to %s.\n", Ship(s),
                    DIST_TO_DOCK, Ship(s2));
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          fuel = 0.05 + Dist * 0.025 * sqrt(s->mass);
          if (s->fuel < fuel) {
            sprintf(buf, "Not enough fuel.\n");
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          if (Size(s) > Hanger(s2)) {
            sprintf(buf, "Mothership does not have %d hanger space available "
                         "to load ship.\n",
                    Size(s));
            notify(Playernum, Governor, buf);
            free(s);
            free(s2);
            continue;
          }
          use_fuel(s, fuel);

          /* remove the ship from whatever scope it is currently in */
          if (s->whatorbits == LEVEL_PLAN)
            remove_sh_plan(s);
          else if (s->whatorbits == LEVEL_STAR)
            remove_sh_star(s);
          else {
            notify(Playernum, Governor,
                   "Ship is not in planet or star scope.\n");
            free(s);
            free(s2);
            continue;
          }
          /* get the target ship again because it had a pointer changed (and put
           * to disk) in the remove routines */
          free(s2);
          (void)getship(&s2, ship2no);
          insert_sh_ship(s, s2);
          /* increase mass of mothership */
          s2->mass += s->mass;
          s2->hanger += Size(s);
          sprintf(buf, "%s landed on %s using %.1f fuel.\n", Ship(s), Ship(s2),
                  fuel);
          notify(Playernum, Governor, buf);
          s->docked = 1;
          putship(s2);
          free(s2);
        }
      } else { /* attempting to land on a planet */
        if (s->docked) {
          sprintf(buf, "%s is docked.\n", Ship(s));
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        sscanf(args[2], "%d,%d", &x, &y);
        if (s->whatorbits != LEVEL_PLAN) {
          sprintf(buf, "%s doesn't orbit a planet.\n", Ship(s));
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!Shipdata[s->type][ABIL_CANLAND]) {
          sprintf(buf, "This ship is not equipped to land.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if ((s->storbits != Dir[Playernum - 1][Governor].snum) ||
            (s->pnumorbits != Dir[Playernum - 1][Governor].pnum)) {
          sprintf(buf, "You have to cs to the planet it orbits.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!speed_rating(s)) {
          sprintf(buf, "This ship is not rated for maneuvering.\n");
          notify(Playernum, Governor, buf);
          free(s);
          continue;
        }
        if (!enufAP(Playernum, Governor, Stars[s->storbits]->AP[Playernum - 1],
                    APcount)) {
          free(s);
          continue;
        }

        getplanet(&p, (int)s->storbits, (int)s->pnumorbits);

        sprintf(buf, "Planet /%s/%s has gravity field of %.2f.\n",
                Stars[s->storbits]->name,
                Stars[s->storbits]->pnames[s->pnumorbits], gravity(p));
        notify(Playernum, Governor, buf);

        sprintf(buf, "Distance to planet: %.2f.\n",
                Dist = sqrt((double)Distsq(Stars[s->storbits]->xpos + p->xpos,
                                           Stars[s->storbits]->ypos + p->ypos,
                                           s->xpos, s->ypos)));
        notify(Playernum, Governor, buf);

        if (Dist > DIST_TO_LAND) {
          sprintf(buf, "%s must be %.3g or closer to the planet (%.2f).\n",
                  Ship(s), DIST_TO_LAND, Dist);
          notify(Playernum, Governor, buf);
          free(s);
          free(p);
          continue;
        }

        fuel = s->mass * gravity(p) * LAND_GRAV_MASS_FACTOR;

        if ((x < 0) || (y < 0) || (x > p->Maxx - 1) || (y > p->Maxy - 1)) {
          sprintf(buf, "Illegal coordinates.\n");
          notify(Playernum, Governor, buf);
          free(s);
          free(p);
          continue;
        }

#ifdef DEFENSE
        /* people who have declared war on you will fire at your landing ship */
        for (i = 1; i <= Num_races; i++)
          if (s->alive && i != Playernum && p->info[i - 1].popn &&
              p->info[i - 1].guns && p->info[i - 1].destruct) {
            alien = races[i - 1];
            if (isset(alien->atwar, (int)s->owner)) {
              /* attack the landing ship */
              strength =
                  MIN((int)p->info[i - 1].guns, (int)p->info[i - 1].destruct);
              if (strength) {
                post(temp, COMBAT);
                notify_star(0, 0, (int)s->owner, (int)s->storbits, temp);
                warn(i, (int)Stars[s->storbits]->governor[i - 1], buf);
                notify((int)s->owner, (int)s->governor, buf);
                p->info[i - 1].destruct -= strength;
              }
            }
          }
        if (!s->alive) {
          putplanet(p, (int)s->storbits, (int)s->pnumorbits);
          putship(s);
          free(p);
          free(s);
          continue;
        }
#endif
        /* check to see if the ship crashes from lack of fuel or damage */
        if (crash(s, fuel)) {
          /* damaged ships stand of chance of crash landing */
          numdest =
              shoot_ship_to_planet(s, p, round_rand((double)(s->destruct) / 3.),
                                   x, y, 1, 0, HEAVY, long_buf, short_buf);
          sprintf(
              buf,
              "BOOM!! %s crashes on sector %d,%d with blast radius of %d.\n",
              Ship(s), x, y, numdest);
          for (i = 1; i <= Num_races; i++)
            if (p->info[i - 1].numsectsowned || i == Playernum)
              warn(i, (int)Stars[s->storbits]->governor[i - 1], buf);
          if (roll)
            sprintf(buf, "Ship damage %d%% (you rolled a %d)\n", (int)s->damage,
                    roll);
          else
            sprintf(buf, "You had %.1ff while the landing required %.1ff\n",
                    s->fuel, fuel);
          notify(Playernum, Governor, buf);
          kill_ship((int)s->owner, s);
        } else {
          s->land_x = x;
          s->land_y = y;
          s->xpos = p->xpos + Stars[s->storbits]->xpos;
          s->ypos = p->ypos + Stars[s->storbits]->ypos;
          use_fuel(s, fuel);
          s->docked = 1;
          s->whatdest = LEVEL_PLAN; /* no destination */
          s->deststar = s->storbits;
          s->destpnum = s->pnumorbits;
        }

        getsector(&sect, p, x, y);

        if (sect->condition == WASTED) {
          sprintf(buf, "Warning: That sector is a wasteland!\n");
          notify(Playernum, Governor, buf);
        } else if (sect->owner && sect->owner != Playernum) {
          Race = races[Playernum - 1];
          alien = races[sect->owner - 1];
          if (!(isset(Race->allied, sect->owner) &&
                isset(alien->allied, Playernum))) {
            sprintf(buf, "You have landed on an alien sector (%s).\n",
                    alien->name);
            notify(Playernum, Governor, buf);
          } else {
            sprintf(buf, "You have landed on allied sector (%s).\n",
                    alien->name);
            notify(Playernum, Governor, buf);
          }
        }
        if (s->whatorbits == LEVEL_UNIV)
          deductAPs(Playernum, Governor, APcount, 0, 1);
        else
          deductAPs(Playernum, Governor, APcount, (int)s->storbits, 0);

        putplanet(p, (int)s->storbits, (int)s->pnumorbits);

        if (numdest)
          putsector(sect, p, x, y);

        /* send messages to anyone there */
        sprintf(buf, "%s observed landing on sector %d,%d,planet /%s/%s.\n",
                Ship(s), s->land_x, s->land_y, Stars[s->storbits]->name,
                Stars[s->storbits]->pnames[s->pnumorbits]);
        for (i = 1; i <= Num_races; i++)
          if (p->info[i - 1].numsectsowned && i != Playernum)
            notify(i, (int)Stars[s->storbits]->governor[i - 1], buf);

        sprintf(buf, "%s landed on planet.\n", Ship(s));
        notify(Playernum, Governor, buf);

        free(sect);
        free(p);
      }
      putship(s);
      free(s);
    } else
      free(s);
}

int crash(shiptype *s, double fuel) {
  roll = 0;

  if (s->fuel < fuel)
    return 1;
  else if ((roll = int_rand(1, 100)) <= (int)s->damage)
    return 1;
  else
    return 0;
}

int docked(shiptype *s) { return (s->docked && s->whatdest == LEVEL_SHIP); }

int overloaded(shiptype *s) {
  return ((s->resource > Max_resource(s)) || (s->fuel > Max_fuel(s)) ||
          (s->popn + s->troops > s->max_crew) ||
          (s->destruct > Max_destruct(s)));
}
