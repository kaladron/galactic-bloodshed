// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file fire.c
/// \brief Fire at ship or planet from ship or planet

import gblib;
import std;

#include "gb/fire.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/config.h"
#include "gb/doship.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/load.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/shootblast.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static void check_overload(Ship *, int, int *);
static void check_retal_strength(Ship *, int *);

namespace {
// check to see if there are any planetary defense networks on the planet
bool has_planet_defense(const shipnum_t shipno, const player_t Playernum) {
  Shiplist shiplist(shipno);
  for (const auto &ship : shiplist) {
    if (ship.alive && ship.type == ShipType::OTYPE_PLANDEF &&
        ship.owner != Playernum) {
      return true;
    }
  }
  return false;
}
}  // namespace

/*! Ship vs ship */
void fire(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount;
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
  shipnum_t fromship;
  shipnum_t toship;
  shipnum_t sh;
  shipnum_t nextshipno;
  Ship *from;
  Ship dummy;
  int strength;
  int maxstrength;
  int retal;
  int damage;

  sh = 0;  // TODO(jeffbailey): No idea what this is, init to 0.

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argv.size() < 3) {
    std::string msg =
        "Syntax: '" + argv[0] + " <ship> <target> [<strength>]'.\n";
    notify(Playernum, Governor, msg);
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, argv[1], *from, &nextshipno) &&
        authorized(Governor, *from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n",
                ship_to_string(*from).c_str());
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
                         stars[from->storbits].AP[Playernum - 1], APcount)) {
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
      auto toshiptmp = string_to_shipnum(argv[1]);
      if (!toshiptmp || *toshiptmp <= 0) {
        g.out << "Bad ship number.\n";
        free(from);
        return;
      }
      toship = *toshiptmp;
      if (toship == fromship) {
        g.out << "Get real.\n";
        free(from);
        continue;
      }
      auto to = getship(toship);
      if (!to) {
        continue;
      }

      /* save defense attack strength for retaliation */
      check_retal_strength(&*to, &retal);
      bcopy(&*to, &dummy, sizeof(Ship));

      if (from->type == ShipType::OTYPE_AFV) {
        if (!landed(*from)) {
          sprintf(buf, "%s isn't landed on a planet!\n",
                  ship_to_string(*from).c_str());
          notify(Playernum, Governor, buf);
          free(from);
          continue;
        }
        if (!landed(*to)) {
          sprintf(buf, "%s isn't landed on a planet!\n",
                  ship_to_string(*from).c_str());
          notify(Playernum, Governor, buf);
          free(from);
          continue;
        }
      }
      if (landed(*from) && landed(*to)) {
        if ((from->storbits != to->storbits) ||
            (from->pnumorbits != to->pnumorbits)) {
          notify(Playernum, Governor,
                 "Landed ships can only attack other "
                 "landed ships if they are on the same "
                 "planet!\n");
          free(from);
          continue;
        }
        const auto p = getplanet(from->storbits, from->pnumorbits);
        if (!adjacent((int)from->land_x, (int)from->land_y, (int)to->land_x,
                      (int)to->land_y, p)) {
          g.out << "You are not adjacent to your target!\n";
          free(from);
          continue;
        }
      }
      if (cew) {
        if (from->fuel < (double)from->cew) {
          sprintf(buf, "You need %d fuel to fire CEWs.\n", from->cew);
          notify(Playernum, Governor, buf);
          free(from);
          continue;
        }
        if (landed(*from) || landed(*to)) {
          notify(Playernum, Governor,
                 "CEWs cannot originate from or targeted "
                 "to ships landed on planets.\n");
          free(from);
          continue;
        }
        sprintf(buf, "CEW strength %d.\n", from->cew);
        notify(Playernum, Governor, buf);
        strength = from->cew / 2;

      } else {
        check_retal_strength(from, &maxstrength);

        if (argv.size() >= 4)
          strength = std::stoi(argv[3]);
        else
          check_retal_strength(from, &strength);

        if (strength > maxstrength) {
          strength = maxstrength;
          sprintf(buf, "%s set to %d\n",
                  laser_on(*from) ? "Laser strength" : "Guns", strength);
          notify(Playernum, Governor, buf);
        }
      }

      /* check to see if there is crystal overloads */
      if (laser_on(*from) || cew) check_overload(from, cew, &strength);

      if (strength <= 0) {
        sprintf(buf, "No attack.\n");
        notify(Playernum, Governor, buf);
        putship(from);
        free(from);
        continue;
      }

      damage =
          shoot_ship_to_ship(from, &*to, strength, cew, 0, long_buf, short_buf);

      if (damage < 0) {
        g.out << "Illegal attack.\n";
        free(from);
        continue;
      }

      if (laser_on(*from) || cew)
        use_fuel(*from, 2.0 * (double)strength);
      else
        use_destruct(*from, strength);

      if (!to->alive) post(short_buf, COMBAT);
      notify_star(Playernum, Governor, from->storbits, short_buf);
      warn(to->owner, to->governor, long_buf);
      notify(Playernum, Governor, long_buf);
      /* defending ship retaliates */

      strength = 0;
      if (retal && damage && to->protect.self) {
        strength = retal;
        if (laser_on(*to)) check_overload(&*to, 0, &strength);

        if ((damage = shoot_ship_to_ship(&dummy, from, strength, 0, 1, long_buf,
                                         short_buf)) >= 0) {
          if (laser_on(*to))
            use_fuel(*to, 2.0 * (double)strength);
          else
            use_destruct(*to, strength);
          if (!from->alive) post(short_buf, COMBAT);
          notify_star(Playernum, Governor, from->storbits, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(to->owner, to->governor, long_buf);
        }
      }
      /* protecting ships retaliate individually if damage was inflicted */
      /* AFVs immune to retaliation of this type */
      if (damage && from->alive && from->type != ShipType::OTYPE_AFV) {
        if (to->whatorbits == ScopeLevel::LEVEL_STAR) /* star level ships */
          sh = stars[to->storbits].ships;
        if (to->whatorbits == ScopeLevel::LEVEL_PLAN) { /* planet level ships */
          const auto p = getplanet(to->storbits, to->pnumorbits);
          sh = p.ships;
        }
        Shiplist shiplist(sh);
        for (auto &ship : shiplist) {
          if (!from->alive) break;
          if (ship.protect.on && (ship.protect.ship == toship) &&
              (ship.protect.ship == toship) && ship.number != fromship &&
              ship.number != toship && ship.alive && ship.active) {
            check_retal_strength(&ship, &strength);
            if (laser_on(ship)) check_overload(&ship, 0, &strength);

            if ((damage = shoot_ship_to_ship(&ship, from, strength, 0, 0,
                                             long_buf, short_buf)) >= 0) {
              if (laser_on(ship))
                use_fuel(ship, 2.0 * (double)strength);
              else
                use_destruct(ship, strength);
              if (!from->alive) post(short_buf, COMBAT);
              notify_star(Playernum, Governor, from->storbits, short_buf);
              notify(Playernum, Governor, long_buf);
              warn(ship.owner, ship.governor, long_buf);
            }
            putship(&ship);
          }
        }
      }
      putship(from);
      putship(&*to);
      deductAPs(g, APcount, from->storbits);

      free(from);
    } else
      free(from);
}

/*! Ship vs planet */
void bombard(const command_t &argv, GameObj &g) {
  int Playernum = g.player;
  int Governor = g.governor;
  ap_t APcount = 1;
  shipnum_t fromship;
  shipnum_t nextshipno;
  Ship *from;
  int strength;
  int maxstrength;
  int x;
  int y;
  int numdest;
  int damage;
  int i;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  if (argv.size() < 2) {
    notify(Playernum, Governor,
           "Syntax: 'bombard <ship> [<x,y> [<strength>]]'.\n");
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);
  while ((fromship = do_shiplist(&from, &nextshipno)))
    if (in_list(Playernum, argv[1], *from, &nextshipno) &&
        authorized(Governor, *from)) {
      if (!from->active) {
        sprintf(buf, "%s is irradiated and inactive.\n",
                ship_to_string(*from).c_str());
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
      if (from->type == ShipType::OTYPE_AFV && !landed(*from)) {
        g.out << "This ship is not landed on the planet.\n";
        free(from);
        continue;
      }
      if (!enufAP(Playernum, Governor, stars[from->storbits].AP[Playernum - 1],
                  APcount)) {
        free(from);
        continue;
      }

      check_retal_strength(from, &maxstrength);

      if (argv.size() > 3)
        strength = std::stoi(argv[3]);
      else
        check_retal_strength(from, &strength);

      if (strength > maxstrength) {
        strength = maxstrength;
        sprintf(buf, "%s set to %d\n",
                laser_on(*from) ? "Laser strength" : "Guns", strength);
        notify(Playernum, Governor, buf);
      }

      /* check to see if there is crystal overload */
      if (laser_on(*from)) check_overload(from, 0, &strength);

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
          g.out << "Illegal sector.\n";
          free(from);
          continue;
        }
      } else {
        x = int_rand(0, (int)p.Maxx - 1);
        y = int_rand(0, (int)p.Maxy - 1);
      }
      if (landed(*from) &&
          !adjacent((int)from->land_x, (int)from->land_y, x, y, p)) {
        g.out << "You are not adjacent to that sector.\n";
        free(from);
        continue;
      }

      bool has_defense = has_planet_defense(p.ships, Playernum);

      if (has_defense && !landed(*from)) {
        g.out << "Target has planetary defense networks.\n";
        g.out << "These have to be eliminated before you can attack sectors.\n";
        free(from);
        continue;
      }

      auto smap = getsmap(p);
      numdest = shoot_ship_to_planet(from, p, strength, x, y, smap, 0, 0,
                                     long_buf, short_buf);
      putsmap(smap, p);

      if (numdest < 0) {
        g.out << "Illegal attack.\n";
        free(from);
        continue;
      }

      if (laser_on(*from))
        use_fuel(*from, 2.0 * (double)strength);
      else
        use_destruct(*from, strength);

      post(short_buf, COMBAT);
      notify_star(Playernum, Governor, from->storbits, short_buf);
      for (i = 1; i <= Num_races; i++)
        if (Nuked[i - 1])
          warn(i, stars[from->storbits].governor[i - 1], long_buf);
      notify(Playernum, Governor, long_buf);

#ifdef DEFENSE
      /* planet retaliates - AFVs are immune to this */
      if (numdest && from->type != ShipType::OTYPE_AFV) {
        damage = 0;
        for (i = 1; i <= Num_races; i++)
          if (Nuked[i - 1] && !p.slaved_to) {
            /* add planet defense strength */
            auto &alien = races[i - 1];
            strength = MIN(p.info[i - 1].destruct, p.info[i - 1].guns);

            p.info[i - 1].destruct -= strength;

            damage = shoot_planet_to_ship(alien, from, strength, long_buf,
                                          short_buf);
            warn(i, stars[from->storbits].governor[i - 1], long_buf);
            notify(Playernum, Governor, long_buf);
            if (!from->alive) post(short_buf, COMBAT);
            notify_star(Playernum, Governor, from->storbits, short_buf);
          }
      }
#endif

      /* protecting ships retaliate individually if damage was inflicted */
      /* AFVs are immune to this */
      if (numdest && from->alive && from->type != ShipType::OTYPE_AFV) {
        Shiplist shiplist(p.ships);
        for (auto ship : shiplist) {
          if (ship.protect.planet && ship.number != fromship && ship.alive &&
              ship.active) {
            if (laser_on(ship)) check_overload(&ship, 0, &strength);

            check_retal_strength(&ship, &strength);

            if ((damage = shoot_ship_to_ship(&ship, from, strength, 0, 0,
                                             long_buf, short_buf)) >= 0) {
              if (laser_on(ship))
                use_fuel(ship, 2.0 * (double)strength);
              else
                use_destruct(ship, strength);
              if (!from->alive) post(short_buf, COMBAT);
              notify_star(Playernum, Governor, from->storbits, short_buf);
              warn(ship.owner, ship.governor, long_buf);
              notify(Playernum, Governor, long_buf);
            }
          }
          if (!from->alive) break;
        }
      }

      /* write the stuff to disk */
      putship(from);
      putplanet(p, stars[from->storbits], (int)from->pnumorbits);
      deductAPs(g, APcount, from->storbits);

      free(from);
    } else
      free(from);
}

#ifdef DEFENSE
/*! Planet vs ship */
void defend(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  int toship;
  int sh;
  Ship *ship;
  Ship dummy;
  int strength;
  int retal;
  int damage;
  int x;
  int y;
  int numdest;

  /* for telegramming and retaliating */
  bzero((char *)Nuked, sizeof(Nuked));

  /* get the planet from the players current scope */
  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to set scope to the planet first.\n";
    return;
  }

  if (argv.size() < 3) {
    notify(Playernum, Governor,
           "Syntax: 'defend <ship> <sector> [<strength>]'.\n");
    return;
  }
  if (Governor && stars[g.snum].governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor,
           "You are not authorized to do that in this system.\n");
    return;
  }
  auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return;
  }
  toship = *toshiptmp;

  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (!p.info[Playernum - 1].numsectsowned) {
    g.out << "You do not occupy any sectors here.\n";
    return;
  }

  if (p.slaved_to && p.slaved_to != Playernum) {
    g.out << "This planet is enslaved.\n";
    return;
  }

  auto to = getship(toship);
  if (!to) {
    return;
  }

  if (to->whatorbits != ScopeLevel::LEVEL_PLAN) {
    g.out << "The ship is not in planet orbit.\n";
    return;
  }

  if (to->storbits != g.snum || to->pnumorbits != g.pnum) {
    g.out << "Target is not in orbit around this planet.\n";
    return;
  }

  if (landed(*to)) {
    g.out << "Planet guns can't fire on landed ships.\n";
    return;
  }

  /* save defense strength for retaliation */
  check_retal_strength(&*to, &retal);
  bcopy(&*to, &dummy, sizeof(Ship));

  sscanf(argv[2].c_str(), "%d,%d", &x, &y);

  if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
    g.out << "Illegal sector.\n";
    return;
  }

  /* check to see if you own the sector */
  auto sect = getsector(p, x, y);
  if (sect.owner != Playernum) {
    g.out << "Nice try.\n";
    return;
  }

  if (argv.size() >= 4)
    strength = std::stoi(argv[3]);
  else
    strength = p.info[Playernum - 1].guns;

  strength = MIN(strength, p.info[Playernum - 1].destruct);
  strength = MIN(strength, p.info[Playernum - 1].guns);

  if (strength <= 0) {
    sprintf(buf, "No attack - %d guns, %dd\n", p.info[Playernum - 1].guns,
            p.info[Playernum - 1].destruct);
    notify(Playernum, Governor, buf);
    return;
  }
  auto &race = races[Playernum - 1];

  damage = shoot_planet_to_ship(race, &*to, strength, long_buf, short_buf);

  if (!to->alive && to->type == ShipType::OTYPE_TOXWC) {
    /* get planet again since toxicity probably has changed */
    p = getplanet(g.snum, g.pnum);
  }

  if (damage < 0) {
    sprintf(buf, "Target out of range  %d!\n", SYSTEMSIZE);
    notify(Playernum, Governor, buf);
    return;
  }

  p.info[Playernum - 1].destruct -= strength;
  if (!to->alive) post(short_buf, COMBAT);
  notify_star(Playernum, Governor, to->storbits, short_buf);
  warn(to->owner, to->governor, long_buf);
  notify(Playernum, Governor, long_buf);

  /* defending ship retaliates */

  strength = 0;
  if (retal && damage && to->protect.self) {
    strength = retal;
    if (laser_on(*to)) check_overload(&*to, 0, &strength);

    auto smap = getsmap(p);
    if ((numdest = shoot_ship_to_planet(&dummy, p, strength, x, y, smap, 0, 0,
                                        long_buf, short_buf)) >= 0) {
      if (laser_on(*to))
        use_fuel(*to, 2.0 * (double)strength);
      else
        use_destruct(*to, strength);

      post(short_buf, COMBAT);
      notify_star(Playernum, Governor, to->storbits, short_buf);
      notify(Playernum, Governor, long_buf);
      warn(to->owner, to->governor, long_buf);
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
        if (laser_on(*ship)) check_overload(ship, 0, &strength);
        check_retal_strength(ship, &strength);

        auto smap = getsmap(p);
        if ((numdest = shoot_ship_to_planet(ship, p, strength, x, y, smap, 0, 0,
                                            long_buf, short_buf)) >= 0) {
          if (laser_on(*ship))
            use_fuel(*ship, 2.0 * (double)strength);
          else
            use_destruct(*ship, strength);
          post(short_buf, COMBAT);
          notify_star(Playernum, Governor, ship->storbits, short_buf);
          notify(Playernum, Governor, long_buf);
          warn(ship->owner, ship->governor, long_buf);
        }
        putsmap(smap, p);
        putship(ship);
      }
      sh = ship->nextship;
      free(ship);
    }
  }

  /* write the ship stuff out to disk */
  putship(&*to);
  putplanet(p, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);
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

  Ship *s;
  shipnum_t shipno;
  shipnum_t nextshipno;

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, argv[1], *s, &nextshipno) &&
        authorized(Governor, *s)) {
      if (s->type != ShipType::STYPE_MINE) {
        g.out << "That is not a mine.\n";
        free(s);
        continue;
      }
      if (!s->on) {
        g.out << "The mine is not activated.\n";
        free(s);
        continue;
      }
      if (s->docked || s->whatorbits == ScopeLevel::LEVEL_SHIP) {
        g.out << "The mine is docked or landed.\n";
        free(s);
        continue;
      }
      free(s);
      domine(shipno, 1);
    } else
      free(s);
}

int retal_strength(Ship *s) {
  int strength = 0;
  int avail = 0;

  if (!s->alive) return 0;
  if (!Shipdata[s->type][ABIL_SPEED] && !landed(*s)) return 0;
  /* land based ships */
  if (!s->popn && (s->type != ShipType::OTYPE_BERS)) return 0;

  if (s->guns == PRIMARY)
    avail = (s->type == ShipType::STYPE_FIGHTER ||
             s->type == ShipType::OTYPE_AFV || s->type == ShipType::OTYPE_BERS)
                ? s->primary
                : MIN(s->popn, s->primary);
  else if (s->guns == SECONDARY)
    avail = (s->type == ShipType::STYPE_FIGHTER ||
             s->type == ShipType::OTYPE_AFV || s->type == ShipType::OTYPE_BERS)
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
    if (abs(fx - tx) <= 1) return 1;
    if (fx == p.Maxx - 1 && tx == 0) return 1;
    if (fx == 0 && tx == p.Maxx - 1) return 1;

    return 0;
  }
  return 0;
}

static void check_overload(Ship *ship, int cew, int *strength) {
  if ((ship->laser && ship->fire_laser) || cew) {
    if (int_rand(0, *strength) >
        (int)((1.0 - .01 * ship->damage) * ship->tech / 2.0)) {
      /* check to see if the ship blows up */
      sprintf(buf,
              "%s: Matter-antimatter EXPLOSION from overloaded crystal on %s\n",
              Dispshiploc(ship), ship_to_string(*ship).c_str());
      kill_ship((int)(ship->owner), ship);
      *strength = 0;
      warn(ship->owner, ship->governor, buf);
      post(buf, COMBAT);
      notify_star(ship->owner, ship->governor, ship->storbits, buf);
    } else if (int_rand(0, *strength) >
               (int)((1.0 - .01 * ship->damage) * ship->tech / 4.0)) {
      sprintf(buf, "%s: Crystal damaged from overloading on %s.\n",
              Dispshiploc(ship), ship_to_string(*ship).c_str());
      ship->fire_laser = 0;
      ship->mounted = 0;
      *strength = 0;
      warn(ship->owner, ship->governor, buf);
    }
  }
}

static void check_retal_strength(Ship *ship, int *strength) {
  *strength = 0;
  if (ship->active && ship->alive) { /* irradiated ships dont retaliate */
    if (laser_on(*ship))
      *strength = MIN(ship->fire_laser, (int)ship->fuel / 2);
    else
      *strength = retal_strength(ship);
  }
}
