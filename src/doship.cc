// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* doship -- do one ship turn. */

#include "doship.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GB_server.h"
#include "VN.h"
#include "buffers.h"
#include "build.h"
#include "doturn.h"
#include "files.h"
#include "files_shl.h"
#include "fire.h"
#include "load.h"
#include "max.h"
#include "moveship.h"
#include "perm.h"
#include "power.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

void doship(shiptype *ship, int update) {
  racetype *Race;
  shiptype *ship2;

  /*ship is active */
  ship->active = 1;

  if (!ship->owner)
    ship->alive = 0;

  if (ship->alive) {
    /* repair radiation */
    if (ship->rad) {
      ship->active = 1;
      /* irradiated ships are immobile.. */
      /* kill off some people */
      /* check to see if ship is active */
      if (success(ship->rad))
        ship->active = 0;
      if (update) {
        ship->popn = round_rand(ship->popn * .80);
        ship->troops = round_rand(ship->troops * .80);
        if (ship->rad >= (int)REPAIR_RATE)
          ship->rad -= int_rand(0, (int)REPAIR_RATE);
        else
          ship->rad -= int_rand(0, (int)ship->rad);
      }
    } else
      ship->active = 1;

    if (!ship->popn && Max_crew(ship) && !ship->docked)
      ship->whatdest = LEVEL_UNIV;

    if (ship->whatorbits != LEVEL_UNIV &&
        Stars[ship->storbits]->nova_stage > 0) {
      /* damage ships from supernovae */
      /* Maarten: modified to take into account MOVES_PER_UPDATE */
      ship->damage += 5 * Stars[ship->storbits]->nova_stage /
                      ((Armor(ship) + 1) * segments);
      if (ship->damage >= 100) {
        kill_ship((int)(ship->owner), ship);
        return;
      }
    }

    if (ship->type == OTYPE_FACTORY && !ship->on) {
      Race = races[ship->owner - 1];
      ship->tech = Race->tech;
    }

    if (ship->active)
      Moveship(ship, update, 1, 0);

    ship->size = ship_size(ship); /* for debugging */

    if (ship->whatorbits == LEVEL_SHIP) {
      (void)getship(&ship2, (int)ship->destshipno);
      if (ship2->owner != ship->owner) {
        ship2->owner = ship->owner;
        ship2->governor = ship->governor;
        putship(ship2);
      }
      free(ship2);
      /* just making sure */
    } else if (ship->whatorbits != LEVEL_UNIV &&
               (ship->popn || ship->type == OTYPE_PROBE)) {
      /* Though I have often used TWCs for exploring, I don't think it is right
       */
      /* to be able to map out worlds with this type of junk. Either a manned
       * ship, */
      /* or a probe, which is designed for this kind of work.  Maarten */
      StarsInhab[ship->storbits] = 1;
      setbit(Stars[ship->storbits]->inhabited, ship->owner);
      setbit(Stars[ship->storbits]->explored, ship->owner);
      if (ship->whatorbits == LEVEL_PLAN) {
        planets[ship->storbits][ship->pnumorbits]
            ->info[ship->owner - 1]
            .explored = 1;
      }
    }

    /* add ships, popn to total count to add AP's */
    if (update) {
      Power[ship->owner - 1].ships_owned++;
      Power[ship->owner - 1].resource += ship->resource;
      Power[ship->owner - 1].fuel += ship->fuel;
      Power[ship->owner - 1].destruct += ship->destruct;
      Power[ship->owner - 1].popn += ship->popn;
      Power[ship->owner - 1].troops += ship->troops;
    }

    if (ship->whatorbits == LEVEL_UNIV) {
      Sdatanumships[ship->owner - 1]++;
      Sdatapopns[ship->owner] += ship->popn;
    } else {
      starnumships[ship->storbits][ship->owner - 1]++;
      /* add popn of ships to popn */
      starpopns[ship->storbits][ship->owner - 1] += ship->popn;
      /* set inhabited for ship */
      /* only if manned or probe.  Maarten */
      if (ship->popn || ship->type == OTYPE_PROBE) {
        StarsInhab[ship->storbits] = 1;
        setbit(Stars[ship->storbits]->inhabited, ship->owner);
        setbit(Stars[ship->storbits]->explored, ship->owner);
      }
    }

    if (ship->active) {
      /* bombard the planet */
      if (can_bombard(ship) && ship->bombard &&
          ship->whatorbits == LEVEL_PLAN && ship->whatdest == LEVEL_PLAN &&
          ship->deststar == ship->storbits &&
          ship->destpnum == ship->pnumorbits) {
        /* ship bombards planet */
        Stinfo[ship->storbits][ship->pnumorbits].inhab = 1;
      }

      /* repair ship by the amount of crew it has */
      /* industrial complexes can repair (robot ships
         and offline factories can't repair) */
      if (ship->damage && Repair(ship))
        do_repair(ship);

      if (update)
        switch (ship->type) { /* do this stuff during updates only*/
        case OTYPE_CANIST:
          do_canister(ship);
          break;
        case OTYPE_GREEN:
          do_greenhouse(ship);
          break;
        case STYPE_MIRROR:
          do_mirror(ship);
          break;
        case STYPE_GOD:
          do_god(ship);
          break;
        case OTYPE_AP:
          do_ap(ship);
          break;
        case OTYPE_VN: /* Von Neumann machine */
        case OTYPE_BERS:
          if (!ship->special.mind.progenitor)
            ship->special.mind.progenitor = 1;
          do_VN(ship);
          break;
        case STYPE_OAP:
          do_oap(ship);
          break;
        case STYPE_HABITAT:
          do_habitat(ship);
          break;
        default:
          break;
        }
      if (ship->type == STYPE_POD)
        do_pod(ship);
    }
  }
}

void domass(shiptype *ship) {
  double rmass;
  int sh;

  rmass = races[ship->owner - 1]->mass;

  sh = ship->ships;
  ship->mass = 0.0;
  ship->hanger = 0;
  while (sh) {
    domass(ships[sh]); /* recursive call */
    ship->mass += ships[sh]->mass;
    ship->hanger += ships[sh]->size;
    sh = ships[sh]->nextship;
  }
  ship->mass += getmass(ship);
  ship->mass += (double)(ship->popn + ship->troops) * rmass;
  ship->mass += (double)ship->destruct * MASS_DESTRUCT;
  ship->mass += ship->fuel * MASS_FUEL;
  ship->mass += (double)ship->resource * MASS_RESOURCE;
}

void doown(shiptype *ship) {
  int sh;
  sh = ship->ships;
  while (sh) {
    doown(ships[sh]); /* recursive call */
    ships[sh]->owner = ship->owner;
    ships[sh]->governor = ship->governor;
    sh = ships[sh]->nextship;
  }
}

void domissile(shiptype *ship) {
  int sh2;
  int bombx, bomby, numdest, pdn, i;
  planettype *p;
  double dist;

  if (!ship->alive || !ship->owner)
    return;
  if (!ship->on || ship->docked)
    return;

  /* check to see if it has arrived at it's destination */
  if (ship->whatdest == LEVEL_PLAN && ship->whatorbits == LEVEL_PLAN &&
      ship->destpnum == ship->pnumorbits) {
    p = planets[ship->storbits][ship->pnumorbits];
    /* check to see if PDNs are present */
    pdn = 0;
    sh2 = p->ships;
    while (sh2 && !pdn) {
      if (ships[sh2]->alive && ships[sh2]->type == OTYPE_PLANDEF) {
        /* attack the PDN instead */
        ship->whatdest = LEVEL_SHIP; /* move missile to PDN for attack */
        ship->xpos = ships[sh2]->xpos;
        ship->ypos = ships[sh2]->ypos;
        ship->destshipno = sh2;
        pdn = sh2;
      }
      sh2 = ships[sh2]->nextship;
    }
    if (!pdn) {
      if (ship->special.impact.scatter) {
        bombx = int_rand(1, (int)p->Maxx) - 1;
        bomby = int_rand(1, (int)p->Maxy) - 1;
      } else {
        bombx = ship->special.impact.x % p->Maxx;
        bomby = ship->special.impact.y % p->Maxy;
      }
      sprintf(buf, "%s dropped on sector %d,%d at planet %s.\n", Ship(ship),
              bombx, bomby, prin_ship_orbits(ship));

      numdest = shoot_ship_to_planet(ship, p, (int)ship->destruct, bombx, bomby,
                                     1, 0, HEAVY, long_buf, short_buf);
      push_telegram((int)ship->owner, (int)ship->governor, long_buf);
      kill_ship((int)ship->owner, ship);
      sprintf(buf, "%s dropped on %s.\n\t%d sectors destroyed.\n", Ship(ship),
              prin_ship_orbits(ship), numdest);
      for (i = 1; i <= Num_races; i++)
        if (p->info[i - 1].numsectsowned && i != ship->owner)
          push_telegram(i, Stars[ship->storbits]->governor[i - 1], buf);
      if (numdest) {
        sprintf(buf, "%s dropped on %s.\n", Ship(ship), prin_ship_orbits(ship));
        post(buf, COMBAT);
      }
    }
  } else if (ship->whatdest == LEVEL_SHIP) {
    sh2 = ship->destshipno;
    dist = sqrt(
        Distsq(ship->xpos, ship->ypos, ships[sh2]->xpos, ships[sh2]->ypos));
    if (dist <= ((double)ship->speed * STRIKE_DISTANCE_FACTOR *
                 (100.0 - (double)ship->damage) / 100.0)) {
      /* do the attack */
      (void)shoot_ship_to_ship(ship, ships[sh2], (int)ship->destruct, 0, 0,
                               long_buf, short_buf);
      push_telegram((int)ship->owner, (int)ship->governor, long_buf);
      push_telegram((int)ships[sh2]->owner, (int)ships[sh2]->governor,
                    long_buf);
      kill_ship((int)ship->owner, ship);
      post(short_buf, COMBAT);
    }
  }
}

void domine(int shipno, int detonate) {
  int sh, sh2, i;
  shiptype *s, *ship;
  planettype *planet;
  racetype *r;

  (void)getship(&ship, shipno);

  if (ship->type != STYPE_MINE || !ship->alive || !ship->owner) {
    free(ship);
    return;
  }
  /* check around and see if we should explode. */
  if (ship->on || detonate) {
    int rad = 0;
    double xd, yd, range;

    switch (ship->whatorbits) {
    case LEVEL_STAR:
      sh = Stars[ship->storbits]->ships;
      break;
    case LEVEL_PLAN:
      getplanet(&planet, (int)ship->storbits, (int)ship->pnumorbits);
      sh = planet->ships;
      free(planet);
      break;
    default:
      free(ship);
      return;
    }
    sh2 = sh;
    /* traverse the list, look for ships that
       are closer than the trigger radius... */
    rad = 0;
    if (!detonate) {
      r = races[ship->owner - 1];
      while (sh && !rad) {
        (void)getship(&s, sh);
        xd = s->xpos - ship->xpos;
        yd = s->ypos - ship->ypos;
        range = sqrt(xd * xd + yd * yd);
        if (!isset(r->allied, s->owner) && (s->owner != ship->owner) &&
            ((int)range <= ship->special.trigger.radius))
          rad = 1;
        else
          sh = s->nextship;
        free(s);
      }
    } else
      rad = 1;

    if (rad) {
      sprintf(buf, "%s detonated at %s\n", Ship(ship), prin_ship_orbits(ship));
      post(buf, COMBAT);
      notify_star((int)ship->owner, (int)ship->governor, 0, (int)ship->storbits,
                  buf);
      sh = sh2;
      while (sh) {
        (void)getship(&s, sh);
        if (sh != shipno && s->alive && (s->type != OTYPE_CANIST) &&
            (s->type != OTYPE_GREEN)) {
          rad = shoot_ship_to_ship(ship, s, (int)(ship->destruct), 0, 0,
                                   long_buf, short_buf);
          if (rad > 0) {
            post(short_buf, COMBAT);
            warn((int)s->owner, (int)s->governor, long_buf);
            putship(s);
          }
        }
        sh = s->nextship;
        free(s);
      }

      /* if the mine is in orbit around a planet, nuke the planet too! */
      if (ship->whatorbits == LEVEL_PLAN) {
        /* pick a random sector to nuke */
        reg int x, y, numdest;
        getplanet(&planet, (int)ship->storbits, (int)ship->pnumorbits);
        if (landed(ship)) {
          x = ship->land_x;
          y = ship->land_y;
        } else {
          x = int_rand(0, (int)planet->Maxx - 1);
          y = int_rand(0, (int)planet->Maxy - 1);
        }
        numdest = shoot_ship_to_planet(ship, planet, (int)(ship->destruct), x,
                                       y, 1, 0, LIGHT, long_buf, short_buf);
        putplanet(planet, (int)ship->storbits, (int)ship->pnumorbits);

        sprintf(telegram_buf, "%s", buf);
        if (numdest > 0) {
          sprintf(buf, " - %d sectors destroyed.", numdest);
          strcat(telegram_buf, buf);
        }
        strcat(telegram_buf, "\n");
        for (i = 1; i <= Num_races; i++)
          if (Nuked[i - 1])
            warn(i, (int)Stars[ship->storbits]->governor[i - 1], telegram_buf);
        notify((int)(ship->owner), (int)ship->governor, telegram_buf);
        free(planet);
      }
      kill_ship((int)(ship->owner), ship);
    }
    putship(ship);
  }
  free(ship);
}

void doabm(shiptype *ship) {
  int sh2;
  int numdest;
  planettype *p;

  if (!ship->alive || !ship->owner)
    return;
  if (!ship->on || !ship->retaliate || !ship->destruct)
    return;

  if (landed(ship)) {
    p = planets[ship->storbits][ship->pnumorbits];
    /* check to see if missiles/mines are present */
    sh2 = p->ships;
    while (sh2 && ship->destruct) {
      if (ships[sh2]->alive && ((ships[sh2]->type == STYPE_MISSILE) ||
                                (ships[sh2]->type == STYPE_MINE)) &&
          (ships[sh2]->owner != ship->owner) &&
          !(isset(races[ship->owner - 1]->allied, ships[sh2]->owner) &&
            isset(races[ships[sh2]->owner - 1]->allied, ship->owner))) {
        /* added last two tests to prevent mutually allied missiles
           getting shot up. */
        /* attack the missile/mine */
        numdest = retal_strength(ship);
        numdest = MIN(numdest, ship->destruct);
        numdest = MIN(numdest, ship->retaliate);
        ship->destruct -= numdest;
        (void)shoot_ship_to_ship(ship, ships[sh2], numdest, 0, 0, long_buf,
                                 short_buf);
        push_telegram((int)(ship->owner), (int)ship->governor, long_buf);
        push_telegram((int)(ships[sh2]->owner), (int)ships[sh2]->governor,
                      long_buf);
        post(short_buf, COMBAT);
      }
      sh2 = ships[sh2]->nextship;
    }
  }
}

void do_repair(shiptype *ship) {
  reg int drep, cost;
  reg double maxrep;

  maxrep = REPAIR_RATE / (double)segments;
  /* stations repair for free, and ships docked with them */
  if (Shipdata[ship->type][ABIL_REPAIR])
    cost = 0;
  else if (ship->docked && ship->whatdest == LEVEL_SHIP &&
           ships[ship->destshipno]->type == STYPE_STATION)
    cost = 0;
  else if (ship->docked && ship->whatorbits == LEVEL_SHIP &&
           ships[ship->destshipno]->type == STYPE_STATION)
    cost = 0;
  else {
    maxrep *= (double)(ship->popn) / (double)ship->max_crew;
    cost = (int)(0.005 * maxrep * Cost(ship));
  }
  if (cost <= ship->resource) {
    use_resource(ship, cost);
    drep = (int)maxrep;
    ship->damage = MAX(0, (int)(ship->damage) - drep);
  } else {
    /* use up all of the ships resources */
    drep = (int)(maxrep * ((double)ship->resource / (int)cost));
    use_resource(ship, ship->resource);
    ship->damage = MAX(0, (int)(ship->damage) - drep);
  }
}

void do_habitat(shiptype *ship) {
  reg int sh;
  int add;
  double fuse;

  /* In v5.0+ Habitats make resources out of fuel */
  if (ship->on) {
    fuse = ship->fuel * ((double)ship->popn / (double)ship->max_crew) *
           (1.0 - .01 * (double)ship->damage);
    add = (int)fuse / 20;
    if (ship->resource + add > ship->max_resource)
      add = ship->max_resource - ship->resource;
    fuse = 20.0 * (double)add;
    rcv_resource(ship, add);
    use_fuel(ship, fuse);

    sh = ship->ships;
    while (sh) {
      if (ships[sh]->type == OTYPE_WPLANT)
        rcv_destruct(ship, do_weapon_plant(ships[sh]));
      sh = ships[sh]->nextship;
    }
  }
  add = round_rand((double)ship->popn * races[ship->owner - 1]->birthrate);
  if (ship->popn + add > Max_crew(ship))
    add = Max_crew(ship) - ship->popn;
  rcv_popn(ship, add, races[ship->owner - 1]->mass);
}

void do_pod(shiptype *ship) {
  reg int i;

  if (ship->whatorbits == LEVEL_STAR) {
    if (ship->special.pod.temperature >= POD_THRESHOLD) {
      i = int_rand(0, (int)Stars[ship->storbits]->numplanets - 1);
      sprintf(telegram_buf, "%s has warmed and exploded at %s\n", Ship(ship),
              prin_ship_orbits(ship));
      if (infect_planet((int)ship->owner, (int)ship->storbits, i)) {
        sprintf(buf, "\tmeta-colony established on %s.",
                Stars[ship->storbits]->pnames[i]);
      } else
        sprintf(buf, "\tno spores have survived.");
      strcat(telegram_buf, buf);
      push_telegram((int)(ship->owner), (int)ship->governor, telegram_buf);
      kill_ship((int)(ship->owner), ship);
    } else
      ship->special.pod.temperature += round_rand(
          (double)Stars[ship->storbits]->temperature / (double)segments);
  } else if (ship->whatorbits == LEVEL_PLAN) {
    if (ship->special.pod.decay >= POD_DECAY) {
      sprintf(telegram_buf, "%s has decayed at %s\n", Ship(ship),
              prin_ship_orbits(ship));
      push_telegram((int)ship->owner, (int)ship->governor, telegram_buf);
      kill_ship((int)ship->owner, ship);
    } else {
      ship->special.pod.decay += round_rand(1.0 / (double)segments);
    }
  }
}

int infect_planet(int who, int star, int p) {
  if (success(SPORE_SUCCESS_RATE)) {
    do_meta_infect(who, planets[star][p]);
    return 1;
  } else
    return 0;
}

void do_meta_infect(int who, planettype *p) {
  int owner, x, y;

  getsmap(Smap, p);
  PermuteSects(p);
  bzero((char *)Sectinfo, sizeof(Sectinfo));
  x = int_rand(0, p->Maxx - 1);
  y = int_rand(0, p->Maxy - 1);
  owner = Sector(*p, x, y).owner;
  if (!owner ||
      (who != owner &&
       (double)int_rand(1, 100) >
           100.0 *
               (1.0 - exp(-((double)(Sector(*p, x, y).troops *
                                     races[owner - 1]->fighters / 50.0)))))) {
    p->info[who - 1].explored = 1;
    p->info[who - 1].numsectsowned += 1;
    Sector(*p, x, y).troops = 0;
    Sector(*p, x, y).popn = races[who - 1]->number_sexes;
    Sector(*p, x, y).owner = who;
    Sector(*p, x, y).condition = Sector(*p, x, y).type;
#ifdef POD_TERRAFORM
    Sector(*p, x, y).condition = races[who - 1]->likesbest;
#endif
    putsmap(Smap, p);
  }
}

void do_canister(shiptype *ship) {
  if (ship->whatorbits == LEVEL_PLAN && !landed(ship)) {
    if (++ship->special.timer.count < DISSIPATE) {
      if (Stinfo[ship->storbits][ship->pnumorbits].temp_add < -90)
        Stinfo[ship->storbits][ship->pnumorbits].temp_add = -100;
      else
        Stinfo[ship->storbits][ship->pnumorbits].temp_add -= 10;
    } else { /* timer expired; destroy canister */
      reg int j = 0;
      kill_ship((int)(ship->owner), ship);
      sprintf(telegram_buf,
              "Canister of dust previously covering %s has dissipated.\n",
              prin_ship_orbits(ship));
      for (j = 1; j <= Num_races; j++)
        if (planets[ship->storbits][ship->pnumorbits]
                ->info[j - 1]
                .numsectsowned)
          push_telegram(j, (int)Stars[ship->storbits]->governor[j - 1],
                        telegram_buf);
    }
  }
}

void do_greenhouse(shiptype *ship) {
  if (ship->whatorbits == LEVEL_PLAN && !landed(ship)) {
    if (++ship->special.timer.count < DISSIPATE) {
      if (Stinfo[ship->storbits][ship->pnumorbits].temp_add > 90)
        Stinfo[ship->storbits][ship->pnumorbits].temp_add = 100;
      else
        Stinfo[ship->storbits][ship->pnumorbits].temp_add += 10;
    } else { /* timer expired; destroy canister */
      reg int j = 0;

      kill_ship((int)(ship->owner), ship);
      sprintf(telegram_buf, "Greenhouse gases at %s have dissipated.\n",
              prin_ship_orbits(ship));
      for (j = 1; j <= Num_races; j++)
        if (planets[ship->storbits][ship->pnumorbits]
                ->info[j - 1]
                .numsectsowned)
          push_telegram(j, (int)Stars[ship->storbits]->governor[j - 1],
                        telegram_buf);
    }
  }
}

void do_mirror(shiptype *ship) {
  switch (ship->special.aimed_at.level) {
  case LEVEL_SHIP: /* ship aimed at is a legal ship now */
    /* if in the same system */
    if ((ship->whatorbits == LEVEL_STAR || ship->whatorbits == LEVEL_PLAN) &&
        (ships[ship->special.aimed_at.shipno] != NULL) &&
        (ships[ship->special.aimed_at.shipno]->whatorbits == LEVEL_STAR ||
         ships[ship->special.aimed_at.shipno]->whatorbits == LEVEL_PLAN) &&
        ship->storbits == ships[ship->special.aimed_at.shipno]->storbits &&
        ships[ship->special.aimed_at.shipno]->alive) {
      shiptype *s;
      reg int i;
      double range;
      s = ships[ship->special.aimed_at.shipno];
      range = sqrt(Distsq(ship->xpos, ship->ypos, s->xpos, s->ypos));
      i = int_rand(0, round_rand((2. / ((double)(Body(s)))) *
                                 (double)(ship->special.aimed_at.intensity) /
                                 (range / PLORBITSIZE + 1.0)));
      sprintf(telegram_buf, "%s aimed at %s\n", Ship(ship), Ship(s));
      s->damage += i;
      if (i) {
        sprintf(buf, "%d%% damage done.\n", i);
        strcat(telegram_buf, buf);
      }
      if (s->damage >= 100) {
        sprintf(buf, "%s DESTROYED!!!\n", Ship(s));
        strcat(telegram_buf, buf);
        kill_ship((int)(ship->owner), s);
      }
      push_telegram((int)s->owner, (int)s->governor, telegram_buf);
      push_telegram((int)ship->owner, (int)ship->governor, telegram_buf);
    }
    break;
  case LEVEL_PLAN: {
    reg int i;
    double range;
    range = sqrt(Distsq(ship->xpos, ship->ypos,
                        Stars[ship->storbits]->xpos +
                            planets[ship->storbits][ship->pnumorbits]->xpos,
                        Stars[ship->storbits]->ypos +
                            planets[ship->storbits][ship->pnumorbits]->ypos));
    if (range > PLORBITSIZE)
      i = PLORBITSIZE * ship->special.aimed_at.intensity / range;
    else
      i = ship->special.aimed_at.intensity;

    i = round_rand(.01 * (100.0 - (double)(ship->damage)) * (double)i);
    Stinfo[ship->storbits][ship->special.aimed_at.pnum].temp_add += i;
  } break;
  case LEVEL_STAR: {
    /* have to be in the same system as the star; otherwise
       it's not too fair.. */
    if (ship->special.aimed_at.snum > 0 &&
        ship->special.aimed_at.snum < Sdata.numstars &&
        ship->whatorbits > LEVEL_UNIV &&
        ship->special.aimed_at.snum == ship->storbits)
      Stars[ship->special.aimed_at.snum]->stability += random() & 01;
  } break;
  case LEVEL_UNIV:
    break;
  }
}

void do_god(shiptype *ship) {
  /* gods have infinite power.... heh heh heh */
  if (races[ship->owner - 1]->God) {
    ship->fuel = Max_fuel(ship);
    ship->destruct = Max_destruct(ship);
    ship->resource = Max_resource(ship);
  }
}

void do_ap(shiptype *ship) {
  racetype *Race;

  /* if landed on planet, change conditions to be like race */
  if (landed(ship) && ship->on) {
    int j, d;
    planettype *p;
    p = planets[ship->storbits][ship->pnumorbits];
    Race = races[ship->owner - 1];
    if (ship->fuel >= 3.0) {
      use_fuel(ship, 3.0);
      for (j = RTEMP + 1; j <= OTHER; j++) {
        d = round_rand(ap_planet_factor(p) * crew_factor(ship) *
                       (double)(Race->conditions[j] - p->conditions[j]));
        if (d)
          p->conditions[j] += d;
      }
    } else if (!ship->notified) {
      ship->notified = 1;
      ship->on = 0;
      msg_OOF(ship);
    }
  }
}

double crew_factor(shiptype *ship) {
  int maxcrew;

  if (!(maxcrew = Shipdata[ship->type][ABIL_MAXCREW]))
    return 0.0;
  return ((double)ship->popn / (double)maxcrew);
}

double ap_planet_factor(planettype *p) {
  double x;

  x = (double)p->Maxx * (double)p->Maxy;
  return (AP_FACTOR / (AP_FACTOR + x));
}

void do_oap(shiptype *ship) {
  /* "indimidate" the planet below, for enslavement purposes. */
  if (ship->whatorbits == LEVEL_PLAN)
    Stinfo[ship->storbits][ship->pnumorbits].intimidated = 1;
}

int do_weapon_plant(shiptype *ship) {
  int maxrate, rate;
  maxrate = (int)(races[ship->owner - 1]->tech / 2.0);

  rate = round_rand(MIN((double)ship->resource / (double)RES_COST_WPLANT,
                        ship->fuel / FUEL_COST_WPLANT) *
                    (1. - .01 * (double)ship->damage) * (double)ship->popn /
                    (double)ship->max_crew);
  rate = MIN(rate, maxrate);
  use_resource(ship, (rate * RES_COST_WPLANT));
  use_fuel(ship, ((double)rate * FUEL_COST_WPLANT));
  return rate;
}
