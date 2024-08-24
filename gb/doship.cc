// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* doship -- do one ship turn. */

import gblib;
import std.compat;

#include "gb/doship.h"

#include <strings.h>

#include <cstdlib>

#include "gb/buffers.h"

static constexpr double ap_planet_factor(const Planet &);
static double crew_factor(Ship &);
static void do_ap(Ship &);
static void do_canister(Ship &);
static void do_greenhouse(Ship &);
static void do_god(Ship &);
static void do_habitat(Ship &);
static void do_meta_infect(int, Planet &);
static void do_mirror(Ship &);
static void do_oap(Ship &);
static void do_pod(Ship &);
static void do_repair(Ship &);
static int infect_planet(int, int, int);

void doship(Ship &ship, int update) {
  /*ship is active */
  ship.active = 1;

  if (!ship.owner) ship.alive = 0;

  if (ship.alive) {
    /* repair radiation */
    if (ship.rad) {
      ship.active = 1;
      /* irradiated ships are immobile.. */
      /* kill off some people */
      /* check to see if ship is active */
      if (success(ship.rad)) ship.active = 0;
      if (update) {
        ship.popn = round_rand(ship.popn * .80);
        ship.troops = round_rand(ship.troops * .80);
        if (ship.rad >= (int)REPAIR_RATE)
          ship.rad -= int_rand(0, (int)REPAIR_RATE);
        else
          ship.rad -= int_rand(0, (int)ship.rad);
      }
    } else
      ship.active = 1;

    if (!ship.popn && max_crew(ship) && !ship.docked)
      ship.whatdest = ScopeLevel::LEVEL_UNIV;

    if (ship.whatorbits != ScopeLevel::LEVEL_UNIV &&
        stars[ship.storbits].nova_stage > 0) {
      /* damage ships from supernovae */
      /* Maarten: modified to take into account MOVES_PER_UPDATE */
      ship.damage +=
          5L * stars[ship.storbits].nova_stage / ((armor(ship) + 1) * segments);
      if (ship.damage >= 100) {
        kill_ship(ship.owner, &ship);
        return;
      }
    }

    if (ship.type == ShipType::OTYPE_FACTORY && !ship.on) {
      auto &race = races[ship.owner - 1];
      ship.tech = race.tech;
    }

    if (ship.active) moveship(ship, update, 1, 0);

    ship.size = ship_size(ship); /* for debugging */

    if (ship.whatorbits == ScopeLevel::LEVEL_SHIP) {
      auto ship2 = getship(ship.destshipno);
      if (ship2->owner != ship.owner) {
        ship2->owner = ship.owner;
        ship2->governor = ship.governor;
        putship(&*ship2);
      }
      /* just making sure */
    } else if (ship.whatorbits != ScopeLevel::LEVEL_UNIV &&
               (ship.popn || ship.type == ShipType::OTYPE_PROBE)) {
      /* Though I have often used TWCs for exploring, I don't think it is right
       */
      /* to be able to map out worlds with this type of junk. Either a manned
       * ship, */
      /* or a probe, which is designed for this kind of work.  Maarten */
      StarsInhab[ship.storbits] = 1;
      setbit(stars[ship.storbits].inhabited, ship.owner);
      setbit(stars[ship.storbits].explored, ship.owner);
      if (ship.whatorbits == ScopeLevel::LEVEL_PLAN) {
        planets[ship.storbits][ship.pnumorbits]->info[ship.owner - 1].explored =
            1;
      }
    }

    /* add ships, popn to total count to add AP's */
    if (update) {
      Power[ship.owner - 1].ships_owned++;
      Power[ship.owner - 1].resource += ship.resource;
      Power[ship.owner - 1].fuel += ship.fuel;
      Power[ship.owner - 1].destruct += ship.destruct;
      Power[ship.owner - 1].popn += ship.popn;
      Power[ship.owner - 1].troops += ship.troops;
    }

    if (ship.whatorbits == ScopeLevel::LEVEL_UNIV) {
      Sdatanumships[ship.owner - 1]++;
      Sdatapopns[ship.owner] += ship.popn;
    } else {
      starnumships[ship.storbits][ship.owner - 1]++;
      /* add popn of ships to popn */
      starpopns[ship.storbits][ship.owner - 1] += ship.popn;
      /* set inhabited for ship */
      /* only if manned or probe.  Maarten */
      if (ship.popn || ship.type == ShipType::OTYPE_PROBE) {
        StarsInhab[ship.storbits] = 1;
        setbit(stars[ship.storbits].inhabited, ship.owner);
        setbit(stars[ship.storbits].explored, ship.owner);
      }
    }

    if (ship.active) {
      /* bombard the planet */
      if (can_bombard(ship) && ship.bombard &&
          ship.whatorbits == ScopeLevel::LEVEL_PLAN &&
          ship.whatdest == ScopeLevel::LEVEL_PLAN &&
          ship.deststar == ship.storbits && ship.destpnum == ship.pnumorbits) {
        /* ship bombards planet */
        Stinfo[ship.storbits][ship.pnumorbits].inhab = 1;
      }

      /* repair ship by the amount of crew it has */
      /* industrial complexes can repair (robot ships
         and offline factories can't repair) */
      if (ship.damage && repair(ship)) do_repair(ship);

      if (update) switch (ship.type) { /* do this stuff during updates only*/
          case ShipType::OTYPE_CANIST:
            do_canister(ship);
            break;
          case ShipType::OTYPE_GREEN:
            do_greenhouse(ship);
            break;
          case ShipType::STYPE_MIRROR:
            do_mirror(ship);
            break;
          case ShipType::STYPE_GOD:
            do_god(ship);
            break;
          case ShipType::OTYPE_AP:
            do_ap(ship);
            break;
          case ShipType::OTYPE_VN: /* Von Neumann machine */
          case ShipType::OTYPE_BERS:
            if (!ship.special.mind.progenitor) ship.special.mind.progenitor = 1;
            do_VN(ship);
            break;
          case ShipType::STYPE_OAP:
            do_oap(ship);
            break;
          case ShipType::STYPE_HABITAT:
            do_habitat(ship);
            break;
          default:
            break;
        }
      if (ship.type == ShipType::STYPE_POD) do_pod(ship);
    }
  }
}

void domass(Ship &ship) {
  double rmass;
  int sh;

  rmass = races[ship.owner - 1].mass;

  sh = ship.ships;
  ship.mass = 0.0;
  ship.hanger = 0;
  while (sh) {
    domass(*ships[sh]); /* recursive call */
    ship.mass += ships[sh]->mass;
    ship.hanger += ships[sh]->size;
    sh = ships[sh]->nextship;
  }
  ship.mass += getmass(ship);
  ship.mass += (double)(ship.popn + ship.troops) * rmass;
  ship.mass += (double)ship.destruct * MASS_DESTRUCT;
  ship.mass += ship.fuel * MASS_FUEL;
  ship.mass += (double)ship.resource * MASS_RESOURCE;
}

void doown(Ship &ship) {
  int sh;
  sh = ship.ships;
  while (sh) {
    doown(*ships[sh]); /* recursive call */
    ships[sh]->owner = ship.owner;
    ships[sh]->governor = ship.governor;
    sh = ships[sh]->nextship;
  }
}

void domissile(Ship &ship) {
  int sh2;
  int bombx;
  int bomby;
  int numdest;
  int pdn;
  int i;
  double dist;

  if (!ship.alive || !ship.owner) return;
  if (!ship.on || ship.docked) return;

  /* check to see if it has arrived at it's destination */
  if (ship.whatdest == ScopeLevel::LEVEL_PLAN &&
      ship.whatorbits == ScopeLevel::LEVEL_PLAN &&
      ship.destpnum == ship.pnumorbits) {
    auto &p = planets[ship.storbits][ship.pnumorbits];
    /* check to see if PDNs are present */
    pdn = 0;
    sh2 = p->ships;
    while (sh2 && !pdn) {
      if (ships[sh2]->alive && ships[sh2]->type == ShipType::OTYPE_PLANDEF) {
        /* attack the PDN instead */
        ship.whatdest =
            ScopeLevel::LEVEL_SHIP; /* move missile to PDN for attack */
        ship.xpos = ships[sh2]->xpos;
        ship.ypos = ships[sh2]->ypos;
        ship.destshipno = sh2;
        pdn = sh2;
      }
      sh2 = ships[sh2]->nextship;
    }
    if (!pdn) {
      if (ship.special.impact.scatter) {
        bombx = int_rand(1, (int)p->Maxx) - 1;
        bomby = int_rand(1, (int)p->Maxy) - 1;
      } else {
        bombx = ship.special.impact.x % p->Maxx;
        bomby = ship.special.impact.y % p->Maxy;
      }
      sprintf(buf, "%s dropped on sector %d,%d at planet %s.\n",
              ship_to_string(ship).c_str(), bombx, bomby,
              prin_ship_orbits(ship).c_str());

      auto smap = getsmap(*p);
      numdest =
          shoot_ship_to_planet(&ship, *p, (int)ship.destruct, bombx, bomby,
                               smap, 0, GTYPE_HEAVY, long_buf, short_buf);
      putsmap(smap, *p);
      push_telegram((int)ship.owner, (int)ship.governor, long_buf);
      kill_ship(ship.owner, &ship);
      sprintf(buf, "%s dropped on %s.\n\t%d sectors destroyed.\n",
              ship_to_string(ship).c_str(), prin_ship_orbits(ship).c_str(),
              numdest);
      for (i = 1; i <= Num_races; i++)
        if (p->info[i - 1].numsectsowned && i != ship.owner)
          push_telegram(i, stars[ship.storbits].governor[i - 1], buf);
      if (numdest) {
        sprintf(buf, "%s dropped on %s.\n", ship_to_string(ship).c_str(),
                prin_ship_orbits(ship).c_str());
        post(buf, NewsType::COMBAT);
      }
    }
  } else if (ship.whatdest == ScopeLevel::LEVEL_SHIP) {
    sh2 = ship.destshipno;
    dist =
        sqrt(Distsq(ship.xpos, ship.ypos, ships[sh2]->xpos, ships[sh2]->ypos));
    if (dist <= ((double)ship.speed * STRIKE_DISTANCE_FACTOR *
                 (100.0 - (double)ship.damage) / 100.0)) {
      /* do the attack */
      (void)shoot_ship_to_ship(&ship, ships[sh2], (int)ship.destruct, 0, 0,
                               long_buf, short_buf);
      push_telegram(ship.owner, ship.governor, long_buf);
      push_telegram(ships[sh2]->owner, ships[sh2]->governor, long_buf);
      kill_ship(ship.owner, &ship);
      post(short_buf, NewsType::COMBAT);
    }
  }
}

void domine(int shipno, int detonate) {
  int i;
  shipnum_t sh;

  auto ship = getship(shipno);

  if (ship->type != ShipType::STYPE_MINE || !ship->alive || !ship->owner) {
    return;
  }
  /* check around and see if we should explode. */
  if (ship->on || detonate) {
    double xd;
    double yd;
    double range;

    switch (ship->whatorbits) {
      case ScopeLevel::LEVEL_STAR:
        sh = stars[ship->storbits].ships;
        break;
      case ScopeLevel::LEVEL_PLAN: {
        const auto planet = getplanet(ship->storbits, ship->pnumorbits);
        sh = planet.ships;
      } break;
      default:
        return;
    }
    /* traverse the list, look for ships that
       are closer than the trigger radius... */
    bool rad = false;
    if (!detonate) {
      auto &r = races[ship->owner - 1];
      Shiplist shiplist(sh);
      for (auto s : shiplist) {
        xd = s.xpos - ship->xpos;
        yd = s.ypos - ship->ypos;
        range = sqrt(xd * xd + yd * yd);
        if (!isset(r.allied, s.owner) && (s.owner != ship->owner) &&
            ((int)range <= ship->special.trigger.radius)) {
          rad = true;
          break;
        }
      }
    } else
      rad = true;

    if (rad) {
      sprintf(buf, "%s detonated at %s\n", ship_to_string(*ship).c_str(),
              prin_ship_orbits(*ship).c_str());
      post(buf, NewsType::COMBAT);
      notify_star(ship->owner, ship->governor, ship->storbits, buf);
      Shiplist shiplist(sh);
      for (auto s : shiplist) {
        if (sh != shipno && s.alive && (s.type != ShipType::OTYPE_CANIST) &&
            (s.type != ShipType::OTYPE_GREEN)) {
          auto damage = shoot_ship_to_ship(&*ship, &s, (int)(ship->destruct), 0,
                                           0, long_buf, short_buf);
          if (damage > 0) {
            post(short_buf, NewsType::COMBAT);
            warn(s.owner, s.governor, long_buf);
            putship(&s);
          }
        }
      }

      /* if the mine is in orbit around a planet, nuke the planet too! */
      if (ship->whatorbits == ScopeLevel::LEVEL_PLAN) {
        /* pick a random sector to nuke */
        int x;
        int y;
        int numdest;
        auto planet = getplanet((int)ship->storbits, (int)ship->pnumorbits);
        if (landed(*ship)) {
          x = ship->land_x;
          y = ship->land_y;
        } else {
          x = int_rand(0, (int)planet.Maxx - 1);
          y = int_rand(0, (int)planet.Maxy - 1);
        }
        auto smap = getsmap(planet);
        numdest =
            shoot_ship_to_planet(&*ship, planet, (int)(ship->destruct), x, y,
                                 smap, 0, GTYPE_LIGHT, long_buf, short_buf);
        putsmap(smap, planet);
        putplanet(planet, stars[ship->storbits], (int)ship->pnumorbits);

        sprintf(telegram_buf, "%s", buf);
        if (numdest > 0) {
          sprintf(buf, " - %d sectors destroyed.", numdest);
          strcat(telegram_buf, buf);
        }
        strcat(telegram_buf, "\n");
        for (i = 1; i <= Num_races; i++)
          if (Nuked[i - 1])
            warn(i, stars[ship->storbits].governor[i - 1], telegram_buf);
        notify((ship->owner), ship->governor, telegram_buf);
      }
      kill_ship((ship->owner), &*ship);
    }
    putship(&*ship);
  }
}

void doabm(Ship &ship) {
  int sh2;
  int numdest;

  if (!ship.alive || !ship.owner) return;
  if (!ship.on || !ship.retaliate || !ship.destruct) return;

  if (landed(ship)) {
    const auto &p = planets[ship.storbits][ship.pnumorbits];
    /* check to see if missiles/mines are present */
    sh2 = p->ships;
    while (sh2 && ship.destruct) {
      if (ships[sh2]->alive &&
          ((ships[sh2]->type == ShipType::STYPE_MISSILE) ||
           (ships[sh2]->type == ShipType::STYPE_MINE)) &&
          (ships[sh2]->owner != ship.owner) &&
          !(isset(races[ship.owner - 1].allied, ships[sh2]->owner) &&
            isset(races[ships[sh2]->owner - 1].allied, ship.owner))) {
        /* added last two tests to prevent mutually allied missiles
           getting shot up. */
        /* attack the missile/mine */
        numdest = retal_strength(ship);
        numdest = MIN(numdest, ship.destruct);
        numdest = MIN(numdest, ship.retaliate);
        ship.destruct -= numdest;
        (void)shoot_ship_to_ship(&ship, ships[sh2], numdest, 0, 0, long_buf,
                                 short_buf);
        push_telegram(ship.owner, ship.governor, long_buf);
        push_telegram(ships[sh2]->owner, ships[sh2]->governor, long_buf);
        post(short_buf, NewsType::COMBAT);
      }
      sh2 = ships[sh2]->nextship;
    }
  }
}

static void do_repair(Ship &ship) {
  int drep;
  int cost;
  double maxrep;

  maxrep = REPAIR_RATE / (double)segments;
  /* stations repair for free, and ships docked with them */
  if (Shipdata[ship.type][ABIL_REPAIR] ||
      (ship.docked && ship.whatdest == ScopeLevel::LEVEL_SHIP &&
       ships[ship.destshipno]->type == ShipType::STYPE_STATION) ||
      (ship.docked && ship.whatorbits == ScopeLevel::LEVEL_SHIP &&
       ships[ship.destshipno]->type == ShipType::STYPE_STATION)) {
    cost = 0;
  } else {
    maxrep *= (double)(ship.popn) / (double)ship.max_crew;
    cost = (int)(0.005 * maxrep * shipcost(ship));
  }
  if (cost <= ship.resource) {
    use_resource(ship, cost);
    drep = (int)maxrep;
    ship.damage = std::max(0, (int)(ship.damage) - drep);
  } else {
    /* use up all of the ships resources */
    drep = (int)(maxrep * ((double)ship.resource / (int)cost));
    use_resource(ship, ship.resource);
    ship.damage = std::max(0, (int)(ship.damage) - drep);
  }
}

static void do_habitat(Ship &ship) {
  int sh;
  int add;
  double fuse;

  /* In v5.0+ Habitats make resources out of fuel */
  if (ship.on) {
    fuse = ship.fuel * ((double)ship.popn / (double)ship.max_crew) *
           (1.0 - .01 * (double)ship.damage);
    add = (int)fuse / 20;
    if (ship.resource + add > ship.max_resource)
      add = ship.max_resource - ship.resource;
    fuse = 20.0 * (double)add;
    rcv_resource(ship, add);
    use_fuel(ship, fuse);

    sh = ship.ships;
    while (sh) {
      if (ships[sh]->type == ShipType::OTYPE_WPLANT)
        rcv_destruct(ship, do_weapon_plant(*ships[sh]));
      sh = ships[sh]->nextship;
    }
  }
  add = round_rand((double)ship.popn * races[ship.owner - 1].birthrate);
  if (ship.popn + add > max_crew(ship)) add = max_crew(ship) - ship.popn;
  rcv_popn(ship, add, races[ship.owner - 1].mass);
}

static void do_pod(Ship &ship) {
  int i;

  if (ship.whatorbits == ScopeLevel::LEVEL_STAR) {
    if (ship.special.pod.temperature >= POD_THRESHOLD) {
      i = int_rand(0, stars[ship.storbits].numplanets - 1);
      sprintf(telegram_buf, "%s has warmed and exploded at %s\n",
              ship_to_string(ship).c_str(), prin_ship_orbits(ship).c_str());
      if (infect_planet((int)ship.owner, (int)ship.storbits, i)) {
        sprintf(buf, "\tmeta-colony established on %s.",
                stars[ship.storbits].pnames[i]);
      } else
        sprintf(buf, "\tno spores have survived.");
      strcat(telegram_buf, buf);
      push_telegram((ship.owner), ship.governor, telegram_buf);
      kill_ship(ship.owner, &ship);
    } else
      ship.special.pod.temperature += round_rand(
          (double)stars[ship.storbits].temperature / (double)segments);
  } else if (ship.whatorbits == ScopeLevel::LEVEL_PLAN) {
    if (ship.special.pod.decay >= POD_DECAY) {
      sprintf(telegram_buf, "%s has decayed at %s\n",
              ship_to_string(ship).c_str(), prin_ship_orbits(ship).c_str());
      push_telegram(ship.owner, ship.governor, telegram_buf);
      kill_ship(ship.owner, &ship);
    } else {
      ship.special.pod.decay += round_rand(1.0 / (double)segments);
    }
  }
}

static int infect_planet(int who, int star, int p) {
  if (success(SPORE_SUCCESS_RATE)) {
    do_meta_infect(who, *planets[star][p]);
    return 1;
  }
  return 0;
}

static void do_meta_infect(int who, Planet &p) {
  int owner;
  int x;
  int y;

  auto smap = getsmap(p);
  // TODO(jeffbailey): I'm pretty certain this bzero is unnecessary, but this is
  // so far away from any other uses of Sectinfo that I'm having trouble proving
  // it.
  bzero((char *)Sectinfo, sizeof(Sectinfo));
  x = int_rand(0, p.Maxx - 1);
  y = int_rand(0, p.Maxy - 1);
  owner = smap.get(x, y).owner;
  if (!owner ||
      (who != owner &&
       (double)int_rand(1, 100) >
           100.0 *
               (1.0 - exp(-((double)(smap.get(x, y).troops *
                                     races[owner - 1].fighters / 50.0)))))) {
    p.info[who - 1].explored = 1;
    p.info[who - 1].numsectsowned += 1;
    smap.get(x, y).troops = 0;
    smap.get(x, y).popn = races[who - 1].number_sexes;
    smap.get(x, y).owner = who;
    smap.get(x, y).condition = smap.get(x, y).type;
    if (POD_TERRAFORM) {
      smap.get(x, y).condition = races[who - 1].likesbest;
    }
    putsmap(smap, p);
  }
}

static void do_canister(Ship &ship) {
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN && !landed(ship)) {
    if (++ship.special.timer.count < DISSIPATE) {
      if (Stinfo[ship.storbits][ship.pnumorbits].temp_add < -90)
        Stinfo[ship.storbits][ship.pnumorbits].temp_add = -100;
      else
        Stinfo[ship.storbits][ship.pnumorbits].temp_add -= 10;
    } else { /* timer expired; destroy canister */
      int j = 0;
      kill_ship(ship.owner, &ship);
      sprintf(telegram_buf,
              "Canister of dust previously covering %s has dissipated.\n",
              prin_ship_orbits(ship).c_str());
      for (j = 1; j <= Num_races; j++)
        if (planets[ship.storbits][ship.pnumorbits]->info[j - 1].numsectsowned)
          push_telegram(j, stars[ship.storbits].governor[j - 1], telegram_buf);
    }
  }
}

static void do_greenhouse(Ship &ship) {
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN && !landed(ship)) {
    if (++ship.special.timer.count < DISSIPATE) {
      if (Stinfo[ship.storbits][ship.pnumorbits].temp_add > 90)
        Stinfo[ship.storbits][ship.pnumorbits].temp_add = 100;
      else
        Stinfo[ship.storbits][ship.pnumorbits].temp_add += 10;
    } else { /* timer expired; destroy canister */
      int j = 0;

      kill_ship(ship.owner, &ship);
      sprintf(telegram_buf, "Greenhouse gases at %s have dissipated.\n",
              prin_ship_orbits(ship).c_str());
      for (j = 1; j <= Num_races; j++)
        if (planets[ship.storbits][ship.pnumorbits]->info[j - 1].numsectsowned)
          push_telegram(j, stars[ship.storbits].governor[j - 1], telegram_buf);
    }
  }
}

static void do_mirror(Ship &ship) {
  switch (ship.special.aimed_at.level) {
    case ScopeLevel::LEVEL_SHIP: /* ship aimed at is a legal ship now */
      /* if in the same system */
      if ((ship.whatorbits == ScopeLevel::LEVEL_STAR ||
           ship.whatorbits == ScopeLevel::LEVEL_PLAN) &&
          (ships[ship.special.aimed_at.shipno] != nullptr) &&
          (ships[ship.special.aimed_at.shipno]->whatorbits ==
               ScopeLevel::LEVEL_STAR ||
           ships[ship.special.aimed_at.shipno]->whatorbits ==
               ScopeLevel::LEVEL_PLAN) &&
          ship.storbits == ships[ship.special.aimed_at.shipno]->storbits &&
          ships[ship.special.aimed_at.shipno]->alive) {
        Ship *s;
        int i;
        double range;
        s = ships[ship.special.aimed_at.shipno];
        range = sqrt(Distsq(ship.xpos, ship.ypos, s->xpos, s->ypos));
        i = int_rand(0, round_rand((2. / ((double)(shipbody(*s)))) *
                                   (double)(ship.special.aimed_at.intensity) /
                                   (range / PLORBITSIZE + 1.0)));
        sprintf(telegram_buf, "%s aimed at %s\n", ship_to_string(ship).c_str(),
                ship_to_string(*s).c_str());
        s->damage += i;
        if (i) {
          sprintf(buf, "%d%% damage done.\n", i);
          strcat(telegram_buf, buf);
        }
        if (s->damage >= 100) {
          sprintf(buf, "%s DESTROYED!!!\n", ship_to_string(*s).c_str());
          strcat(telegram_buf, buf);
          kill_ship(ship.owner, s);
        }
        push_telegram(s->owner, s->governor, telegram_buf);
        push_telegram(ship.owner, ship.governor, telegram_buf);
      }
      break;
    case ScopeLevel::LEVEL_PLAN: {
      int i;
      double range;
      range = sqrt(Distsq(ship.xpos, ship.ypos,
                          stars[ship.storbits].xpos +
                              planets[ship.storbits][ship.pnumorbits]->xpos,
                          stars[ship.storbits].ypos +
                              planets[ship.storbits][ship.pnumorbits]->ypos));
      if (range > PLORBITSIZE)
        i = PLORBITSIZE * ship.special.aimed_at.intensity / range;
      else
        i = ship.special.aimed_at.intensity;

      i = round_rand(.01 * (100.0 - (double)(ship.damage)) * (double)i);
      Stinfo[ship.storbits][ship.special.aimed_at.pnum].temp_add += i;
    } break;
    case ScopeLevel::LEVEL_STAR: {
      /* have to be in the same system as the star; otherwise
         it's not too fair.. */
      if (ship.special.aimed_at.snum > 0 &&
          ship.special.aimed_at.snum < Sdata.numstars &&
          ship.whatorbits > ScopeLevel::LEVEL_UNIV &&
          ship.special.aimed_at.snum == ship.storbits)
        stars[ship.special.aimed_at.snum].stability += random() & 01;
    } break;
    case ScopeLevel::LEVEL_UNIV:
      break;
  }
}

static void do_god(Ship &ship) {
  /* gods have infinite power.... heh heh heh */
  if (races[ship.owner - 1].God) {
    ship.fuel = max_fuel(ship);
    ship.destruct = max_destruct(ship);
    ship.resource = max_resource(ship);
  }
}

static void do_ap(Ship &ship) {
  /* if landed on planet, change conditions to be like race */
  if (landed(ship) && ship.on) {
    int j;
    int d;
    // TODO(jeffbailey): Not obvious here how the modified planet is saved to
    // disk
    auto &p = planets[ship.storbits][ship.pnumorbits];
    auto &race = races[ship.owner - 1];
    if (ship.fuel >= 3.0) {
      use_fuel(ship, 3.0);
      for (j = RTEMP + 1; j <= OTHER; j++) {
        d = round_rand(ap_planet_factor(*p) * crew_factor(ship) *
                       (double)(race.conditions[j] - p->conditions[j]));
        if (d) p->conditions[j] += d;
      }
    } else if (!ship.notified) {
      ship.notified = 1;
      ship.on = 0;
      msg_OOF(ship);
    }
  }
}

static double crew_factor(Ship &ship) {
  int maxcrew;

  if (!(maxcrew = Shipdata[ship.type][ABIL_MAXCREW])) return 0.0;
  return ((double)ship.popn / (double)maxcrew);
}

static constexpr double ap_planet_factor(const Planet &p) {
  double x = p.Maxx * p.Maxy;
  return (AP_FACTOR / (AP_FACTOR + x));
}

static void do_oap(Ship &ship) {
  /* "indimidate" the planet below, for enslavement purposes. */
  if (ship.whatorbits == ScopeLevel::LEVEL_PLAN)
    Stinfo[ship.storbits][ship.pnumorbits].intimidated = 1;
}

int do_weapon_plant(Ship &ship) {
  int maxrate;
  int rate;
  maxrate = (int)(races[ship.owner - 1].tech / 2.0);

  rate = round_rand(MIN((double)ship.resource / (double)RES_COST_WPLANT,
                        ship.fuel / FUEL_COST_WPLANT) *
                    (1. - .01 * (double)ship.damage) * (double)ship.popn /
                    (double)ship.max_crew);
  rate = std::min(rate, maxrate);
  use_resource(ship, (rate * RES_COST_WPLANT));
  use_fuel(ship, ((double)rate * FUEL_COST_WPLANT));
  return rate;
}
