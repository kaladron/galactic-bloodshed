// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  order.c -- give orders to ship */

#include "order.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GB_server.h"
#include "buffers.h"
#include "build.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "load.h"
#include "moveship.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tweakables.h"
#include "vars.h"

static std::string prin_aimed_at(const Ship &);
static void mk_expl_aimed_at(int, int, Ship *);
static void DispOrdersHeader(int, int);
static void DispOrders(int, int, Ship *);
static void give_orders(GameObj &, const command_t &, int, Ship *);

void order(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 1;
  shipnum_t shipno, nextshipno;
  Ship *ship;

  if (argv.size() == 1) { /* display all ship orders */
    DispOrdersHeader(Playernum, Governor);
    nextshipno = start_shiplist(g, "test");
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (ship->owner == Playernum && authorized(Governor, ship)) {
        DispOrders(Playernum, Governor, ship);
        free(ship);
      } else
        free(ship);
  } else if (argv.size() >= 2) {
    DispOrdersHeader(Playernum, Governor);
    nextshipno = start_shiplist(g, argv[1].c_str());
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (in_list(Playernum, argv[1].c_str(), ship, &nextshipno) &&
          authorized(Governor, ship)) {
        if (argv.size() > 2) give_orders(g, argv, APcount, ship);
        DispOrders(Playernum, Governor, ship);
        free(ship);
      } else
        free(ship);
  } else
    g.out << "I don't understand what you mean.\n";
}

// TODO(jeffbailey): We take in a non-zero APcount, and do nothing with it!
static void give_orders(GameObj &g, const command_t &argv, int /* APcount */,
                        Ship *ship) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int j;
  placetype where, pl;
  Ship *tmpship;

  if (!ship->active) {
    sprintf(buf, "%s is irradiated (%d); it cannot be given orders.\n",
            ship_to_string(*ship).c_str(), ship->rad);
    notify(Playernum, Governor, buf);
    return;
  }
  if (ship->type != OTYPE_TRANSDEV && !ship->popn && Max_crew(ship)) {
    sprintf(buf, "%s has no crew and is not a robotic ship.\n",
            ship_to_string(*ship).c_str());
    notify(Playernum, Governor, buf);
    return;
  }

  if (argv[2] == "defense") {
    if (can_bombard(ship)) {
      if (argv[3] == "off")
        ship->protect.planet = 0;
      else
        ship->protect.planet = 1;
    } else {
      notify(Playernum, Governor,
             "That ship cannot be assigned those orders.\n");
      return;
    }
  } else if (argv[2] == "scatter") {
    if (ship->type != STYPE_MISSILE) {
      g.out << "Only missiles can be given this order.\n";
      return;
    }
    ship->special.impact.scatter = 1;
  } else if (argv[2] == "impact") {
    int x, y;
    if (ship->type != STYPE_MISSILE) {
      notify(Playernum, Governor,
             "Only missiles can be designated for this.\n");
      return;
    }
    sscanf(argv[3].c_str(), "%d,%d", &x, &y);
    ship->special.impact.x = x;
    ship->special.impact.y = y;
    ship->special.impact.scatter = 0;
  } else if (argv[2] == "jump") {
    if (ship->docked) {
      notify(Playernum, Governor,
             "That ship is docked. Use 'launch' or 'undock' first.\n");
      return;
    }
    if (ship->hyper_drive.has) {
      if (argv[3] == "off")
        ship->hyper_drive.on = 0;
      else {
        if (ship->whatdest != ScopeLevel::LEVEL_STAR &&
            ship->whatdest != ScopeLevel::LEVEL_PLAN) {
          g.out << "Destination must be star or planet.\n";
          return;
        }
        ship->hyper_drive.on = 1;
        ship->navigate.on = 0;
        if (ship->mounted) {
          ship->hyper_drive.charge = 1;
          ship->hyper_drive.ready = 1;
        }
      }
    } else {
      notify(Playernum, Governor,
             "This ship does not have hyper drive capability.\n");
      return;
    }
  } else if (argv[2] == "protect") {
    if (argv.size() > 3)
      sscanf(argv[3].c_str() + (argv[3][0] == '#'), "%d", &j);
    else
      j = 0;
    if (j == ship->number) {
      g.out << "You can't do that.\n";
      return;
    }
    if (can_bombard(ship)) {
      if (!j) {
        ship->protect.on = 0;
      } else {
        ship->protect.on = 1;
        ship->protect.ship = j;
      }
    } else {
      g.out << "That ship cannot protect.\n";
      return;
    }
  } else if (argv[2] == "navigate") {
    if (argv.size() >= 5) {
      ship->navigate.on = 1;
      ship->navigate.bearing = atoi(argv[3].c_str());
      ship->navigate.turns = atoi(argv[4].c_str());
    } else
      ship->navigate.on = 0;
    if (ship->hyper_drive.on) ship->hyper_drive.on = 0;
  } else if (argv[2] == "switch") {
    if (ship->type == OTYPE_FACTORY) {
      g.out << "Use \"on\" to bring factory online.\n";
      return;
    }
    if (has_switch(ship)) {
      if (ship->whatorbits == ScopeLevel::LEVEL_SHIP) {
        g.out << "That ship is being transported.\n";
        return;
      }
      ship->on = !ship->on;
    } else {
      sprintf(buf, "That ship does not have an on/off setting.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    if (ship->on) {
      switch (ship->type) {
        case STYPE_MINE:
          g.out << "Mine armed and ready.\n";
          break;
        case OTYPE_TRANSDEV:
          g.out << "Transporter ready to receive.\n";
          break;
        default:
          break;
      }
    } else {
      switch (ship->type) {
        case STYPE_MINE:
          g.out << "Mine disarmed.\n";
          break;
        case OTYPE_TRANSDEV:
          g.out << "No longer receiving.\n";
          break;
        default:
          break;
      }
    }
  } else if (argv[2] == "destination") {
    if (speed_rating(ship)) {
      if (ship->docked) {
        notify(Playernum, Governor,
               "That ship is docked; use undock or launch first.\n");
        return;
      }
      where = Getplace(g, argv[3].c_str(), 1);
      if (!where.err) {
        if (where.level == ScopeLevel::LEVEL_SHIP) {
          (void)getship(&tmpship, where.shipno);
          if (!followable(ship, tmpship)) {
            notify(Playernum, Governor,
                   "Warning: that ship is out of range.\n");
            free(tmpship);
            return;
          }
          free(tmpship);
          ship->destshipno = where.shipno;
          ship->whatdest = ScopeLevel::LEVEL_SHIP;
        } else {
          /* to foil cheaters */
          if (where.level != ScopeLevel::LEVEL_UNIV &&
              ((ship->storbits != where.snum) &&
               where.level != ScopeLevel::LEVEL_STAR) &&
              isclr(Stars[where.snum]->explored, ship->owner)) {
            g.out << "You haven't explored this system.\n";
            return;
          }
          ship->whatdest = where.level;
          ship->deststar = where.snum;
          ship->destpnum = where.pnum;
        }
      } else
        return;
    } else {
      g.out << "That ship cannot be launched.\n";
      return;
    }
  } else if (argv[2] == "evade") {
    if (Max_crew(ship) && Max_speed(ship)) {
      if (argv[3] == "on")
        ship->protect.evade = 1;
      else if (argv[3] == "off")
        ship->protect.evade = 0;
    } else
      return;
  } else if (argv[2] == "bombard") {
    if (ship->type != OTYPE_OMCL) {
      if (can_bombard(ship)) {
        if (argv[3] == "off")
          ship->bombard = 0;
        else if (argv[3] == "on")
          ship->bombard = 1;
      } else
        notify(Playernum, Governor,
               "This type of ship cannot be set to retaliate.\n");
    }
  } else if (argv[2] == "retaliate") {
    if (ship->type != OTYPE_OMCL) {
      if (can_bombard(ship)) {
        if (argv[3] == "off")
          ship->protect.self = 0;
        else if (argv[3] == "on")
          ship->protect.self = 1;
      } else
        notify(Playernum, Governor,
               "This type of ship cannot be set to retaliate.\n");
    }
  } else if (argv[2] == "focus") {
    if (ship->laser) {
      if (argv[3] == "on")
        ship->focus = 1;
      else
        ship->focus = 0;
    } else
      g.out << "No laser.\n";
  } else if (argv[2] == "laser") {
    if (ship->laser) {
      if (can_bombard(ship)) {
        if (ship->mounted) {
          if (argv[3] == "on")
            ship->fire_laser = atoi(argv[4].c_str());
          else
            ship->fire_laser = 0;
        } else
          g.out << "You do not have a crystal mounted.\n";
      } else
        notify(Playernum, Governor,
               "This type of ship cannot be set to retaliate.\n");
    } else
      notify(Playernum, Governor,
             "This ship is not equipped with combat lasers.\n");
  } else if (argv[2] == "merchant") {
    if (argv[3] == "off")
      ship->merchant = 0;
    else {
      j = atoi(argv[3].c_str());
      if (j < 0 || j > MAX_ROUTES) {
        g.out << "Bad route number.\n";
        return;
      }
      ship->merchant = j;
    }
  } else if (argv[2] == "speed") {
    if (speed_rating(ship)) {
      j = atoi(argv[3].c_str());
      if (j < 0) {
        g.out << "Specify a positive speed.\n";
        return;
      } else {
        if (j > speed_rating(ship)) j = speed_rating(ship);
        ship->speed = j;
      }
    } else {
      g.out << "This ship does not have a speed rating.\n";
      return;
    }
  } else if (argv[2] == "salvo") {
    if (can_bombard(ship)) {
      j = atoi(argv[3].c_str());
      if (j < 0) {
        g.out << "Specify a positive number of guns.\n";
        return;
      } else {
        if (ship->guns == PRIMARY && j > ship->primary)
          j = ship->primary;
        else if (ship->guns == SECONDARY && j > ship->secondary)
          j = ship->secondary;
        else if (ship->guns == GTYPE_NONE)
          j = 0;

        ship->retaliate = j;
      }
    } else {
      g.out << "This ship cannot be set to retaliate.\n";
      return;
    }
  } else if (argv[2] == "primary") {
    if (ship->primary) {
      if (argv.size() < 4) {
        ship->guns = PRIMARY;
        if (ship->retaliate > ship->primary) ship->retaliate = ship->primary;
      } else {
        j = atoi(argv[3].c_str());
        if (j < 0) {
          notify(Playernum, Governor,
                 "Specify a nonnegative number of guns.\n");
          return;
        } else {
          if (j > ship->primary) j = ship->primary;
          ship->retaliate = j;
          ship->guns = PRIMARY;
        }
      }
    } else {
      g.out << "This ship does not have primary guns.\n";
      return;
    }
  } else if (argv[2] == "secondary") {
    if (ship->secondary) {
      if (argv.size() < 4) {
        ship->guns = SECONDARY;
        if (ship->retaliate > ship->secondary)
          ship->retaliate = ship->secondary;
      } else {
        j = atoi(argv[3].c_str());
        if (j < 0) {
          notify(Playernum, Governor,
                 "Specify a nonnegative number of guns.\n");
          return;
        } else {
          if (j > ship->secondary) j = ship->secondary;
          ship->retaliate = j;
          ship->guns = SECONDARY;
        }
      }
    } else {
      g.out << "This ship does not have secondary guns.\n";
      return;
    }
  } else if (argv[2] == "explosive") {
    switch (ship->type) {
      case STYPE_MINE:
      case OTYPE_GR:
        ship->mode = 0;
        break;
      default:
        return;
    }
  } else if (argv[2] == "radiative") {
    switch (ship->type) {
      case STYPE_MINE:
      case OTYPE_GR:
        ship->mode = 1;
        break;
      default:
        return;
    }
  } else if (argv[2] == "move") {
    if ((ship->type == OTYPE_TERRA) || (ship->type == OTYPE_PLOW)) {
      std::string moveseq;
      if (argv.size() > 3) {
        moveseq = argv[3];
      } else { /* The move list might be empty.. */
        moveseq = "5";
      }
      for (int i = 0; i < moveseq.size(); ++i) {
        /* Make sure the list of moves is short enough. */
        if (i == SHIP_NAMESIZE - 1) {
          sprintf(buf, "Warning: that is more than %d moves.\n",
                  SHIP_NAMESIZE - 1);
          notify(Playernum, Governor, buf);
          notify(Playernum, Governor,
                 "These move orders have been truncated.\n");
          moveseq.resize(i);
          break;
        }
        /* Make sure this move is OK. */
        if ((moveseq[i] == 'c') || (moveseq[i] == 's')) {
          if ((i == 0) && (moveseq[0] == 'c')) {
            notify(Playernum, Governor,
                   "Cycling move orders can not be empty!\n");
            return;
          }
          if (moveseq[i + 1]) {
            sprintf(buf,
                    "Warning: '%c' should be the last character in the "
                    "move order.\n",
                    moveseq[i]);
            notify(Playernum, Governor, buf);
            notify(Playernum, Governor,
                   "These move orders have been truncated.\n");
            moveseq.resize(i + 1);
            break;
          }
        } else if ((moveseq[i] < '1') || ('9' < moveseq[i])) {
          sprintf(buf, "'%c' is not a valid move direction.\n", moveseq[i]);
          notify(Playernum, Governor, buf);
          return;
        }
      }
      strcpy(ship->shipclass, moveseq.c_str());
      /* This is the index keeping track of which order in shipclass is next. */
      ship->special.terraform.index = 0;
    } else {
      notify(Playernum, Governor,
             "That ship is not a terraformer or a space plow.\n");
      return;
    }
  } else if (argv[2] == "trigger") {
    if (ship->type == STYPE_MINE) {
      if (atoi(argv[3].c_str()) < 0)
        ship->special.trigger.radius = 0;
      else
        ship->special.trigger.radius = atoi(argv[3].c_str());
    } else {
      notify(Playernum, Governor,
             "This ship cannot be assigned a trigger radius.\n");
      return;
    }
  } else if (argv[2] == "transport") {
    if (ship->type == OTYPE_TRANSDEV) {
      ship->special.transport.target = atoi(argv[3].c_str());
      if (ship->special.transport.target == ship->number) {
        notify(Playernum, Governor,
               "A transporter cannot transport to itself.");
        ship->special.transport.target = 0;
      } else {
        sprintf(buf, "Target ship is %d.\n", ship->special.transport.target);
        notify(Playernum, Governor, buf);
      }
    } else {
      g.out << "This ship is not a transporter.\n";
      return;
    }
  } else if (argv[2] == "aim") {
    if (can_aim(ship)) {
      if (ship->type == OTYPE_GTELE || ship->type == OTYPE_TRACT ||
          ship->fuel >= FUEL_MANEUVER) {
        if (ship->type == STYPE_MIRROR && ship->docked) {
          sprintf(buf, "docked; use undock or launch first.\n");
          notify(Playernum, Governor, buf);
          return;
        }
        pl = Getplace(g, argv[3].c_str(), 1);
        if (pl.err) {
          g.out << "Error in destination.\n";
          return;
        } else {
          ship->special.aimed_at.level = pl.level;
          ship->special.aimed_at.pnum = pl.pnum;
          ship->special.aimed_at.snum = pl.snum;
          ship->special.aimed_at.shipno = pl.shipno;
          if (ship->type != OTYPE_TRACT && ship->type != OTYPE_GTELE)
            use_fuel(ship, FUEL_MANEUVER);
          if (ship->type == OTYPE_GTELE || ship->type == OTYPE_STELE)
            mk_expl_aimed_at(Playernum, Governor, ship);
          sprintf(buf, "Aimed at %s\n", prin_aimed_at(*ship).c_str());
          notify(Playernum, Governor, buf);
        }
      } else {
        sprintf(buf, "Not enough maneuvering fuel (%.2f).\n", FUEL_MANEUVER);
        notify(Playernum, Governor, buf);
        return;
      }
    } else {
      g.out << "You can't aim that kind of ship.\n";
      return;
    }
  } else if (argv[2] == "intensity") {
    if (ship->type == STYPE_MIRROR) {
      ship->special.aimed_at.intensity =
          std::max(0, std::min(100, atoi(argv[3].c_str())));
    }
  } else if (argv[2] == "on") {
    if (!has_switch(ship)) {
      notify(Playernum, Governor,
             "This ship does not have an on/off setting.\n");
      return;
    }
    if (ship->damage && ship->type != OTYPE_FACTORY) {
      g.out << "Damaged ships cannot be activated.\n";
      return;
    }
    if (ship->on) {
      g.out << "This ship is already activated.\n";
      return;
    }
    if (ship->type == OTYPE_FACTORY) {
      unsigned int oncost;
      if (ship->whatorbits == ScopeLevel::LEVEL_SHIP) {
        Ship *s2;
        int hangerneeded;

        (void)getship(&s2, (int)ship->destshipno);
        if (s2->type == STYPE_HABITAT) {
          oncost = HAB_FACT_ON_COST * ship->build_cost;
          if (s2->resource < oncost) {
            sprintf(buf,
                    "You don't have %d resources on Habitat #%lu to "
                    "activate this factory.\n",
                    oncost, ship->destshipno);
            notify(Playernum, Governor, buf);
            free(s2);
            return;
          }
          hangerneeded = (1 + (int)(HAB_FACT_SIZE * (double)ship_size(ship))) -
                         ((s2->max_hanger - s2->hanger) + ship->size);
          if (hangerneeded > 0) {
            sprintf(
                buf,
                "Not enough hanger space free on Habitat #%lu. Need %d more.\n",
                ship->destshipno, hangerneeded);
            notify(Playernum, Governor, buf);
            free(s2);
            return;
          }
          s2->resource -= oncost;
          s2->hanger -= ship->size;
          ship->size = 1 + (int)(HAB_FACT_SIZE * (double)ship_size(ship));
          s2->hanger += ship->size;
          putship(s2);
          free(s2);
        } else {
          notify(Playernum, Governor,
                 "The factory is currently being transported.\n");
          free(s2);
          return;
        }
      } else if (!landed(ship)) {
        g.out << "You cannot activate the factory here.\n";
        return;
      } else {
        auto planet = getplanet(ship->deststar, ship->destpnum);
        oncost = 2 * ship->build_cost;
        if (planet.info[Playernum - 1].resource < oncost) {
          sprintf(buf,
                  "You don't have %d resources on the planet to activate "
                  "this factory.\n",
                  oncost);
          notify(Playernum, Governor, buf);
          return;
        } else {
          planet.info[Playernum - 1].resource -= oncost;
          putplanet(planet, Stars[ship->deststar], (int)ship->destpnum);
        }
      }
      sprintf(buf, "Factory activated at a cost of %d resources.\n", oncost);
      notify(Playernum, Governor, buf);
    }
    ship->on = 1;
  } else if (argv[2] == "off") {
    if (ship->type == OTYPE_FACTORY && ship->on) {
      notify(Playernum, Governor,
             "You can't deactivate a factory once it's "
             "online. Consider using 'scrap'.\n");
      return;
    }
    ship->on = 0;
  }
  ship->notified = 0;
  putship(ship);
}

static std::string prin_aimed_at(const Ship &ship) {
  placetype targ;

  targ.level = ship.special.aimed_at.level;
  targ.snum = ship.special.aimed_at.snum;
  targ.pnum = ship.special.aimed_at.pnum;
  targ.shipno = ship.special.aimed_at.shipno;
  return Dispplace(targ);
}

std::string prin_ship_dest(const Ship &ship) {
  placetype dest;

  dest.level = ship.whatdest;
  dest.snum = ship.deststar;
  dest.pnum = ship.destpnum;
  dest.shipno = ship.destshipno;
  return Dispplace(dest);
}

/*
 * mark wherever the ship is aimed at, as explored by the owning player.
 */
static void mk_expl_aimed_at(int Playernum, int Governor, Ship *s) {
  double dist;
  startype *str;
  double xf, yf;

  str = Stars[s->special.aimed_at.snum];

  xf = s->xpos;
  yf = s->ypos;

  switch (s->special.aimed_at.level) {
    case ScopeLevel::LEVEL_UNIV:
      sprintf(buf, "There is nothing out here to aim at.");
      notify(Playernum, Governor, buf);
      break;
    case ScopeLevel::LEVEL_STAR:
      sprintf(buf, "Star %s ", prin_aimed_at(*s).c_str());
      notify(Playernum, Governor, buf);
      if ((dist = sqrt(Distsq(xf, yf, str->xpos, str->ypos))) <=
          tele_range((int)s->type, s->tech)) {
        getstar(&str, (int)s->special.aimed_at.snum);
        setbit(str->explored, Playernum);
        putstar(str, (int)s->special.aimed_at.snum);
        sprintf(buf, "Surveyed, distance %g.\n", dist);
        notify(Playernum, Governor, buf);
        free(str);
      } else {
        sprintf(buf, "Too far to see (%g, max %g).\n", dist,
                tele_range((int)s->type, s->tech));
        notify(Playernum, Governor, buf);
      }
      break;
    case ScopeLevel::LEVEL_PLAN: {
      sprintf(buf, "Planet %s ", prin_aimed_at(*s).c_str());
      notify(Playernum, Governor, buf);
      auto p = getplanet(s->special.aimed_at.snum, s->special.aimed_at.pnum);
      if ((dist =
               sqrt(Distsq(xf, yf, str->xpos + p.xpos, str->ypos + p.ypos))) <=
          tele_range((int)s->type, s->tech)) {
        setbit(str->explored, Playernum);
        p.info[Playernum - 1].explored = 1;
        putplanet(p, Stars[s->special.aimed_at.snum],
                  (int)s->special.aimed_at.pnum);
        sprintf(buf, "Surveyed, distance %g.\n", dist);
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "Too far to see (%g, max %g).\n", dist,
                tele_range((int)s->type, s->tech));
        notify(Playernum, Governor, buf);
      }
    } break;
    case ScopeLevel::LEVEL_SHIP:
      sprintf(buf, "You can't see anything of use there.\n");
      notify(Playernum, Governor, buf);
      break;
  }
}

static void DispOrdersHeader(int Playernum, int Governor) {
  notify(Playernum, Governor,
         "    #       name       sp orbits     destin     options\n");
}

static void DispOrders(int Playernum, int Governor, Ship *ship) {
  double distfac;

  if (ship->owner != Playernum || !authorized(Governor, ship) || !ship->alive)
    return;

  if (ship->docked)
    if (ship->whatdest == ScopeLevel::LEVEL_SHIP)
      sprintf(temp, "D#%lu", ship->destshipno);
    else
      sprintf(temp, "L%2d,%-2d", ship->land_x, ship->land_y);
  else
    strcpy(temp, prin_ship_dest(*ship).c_str());

  sprintf(buf, "%5lu %c %14.14s %c%1u %-10s %-10.10s ", ship->number,
          Shipltrs[ship->type], ship->name,
          ship->hyper_drive.has ? (ship->mounted ? '+' : '*') : ' ',
          ship->speed, Dispshiploc_brief(ship), temp);

  if (ship->hyper_drive.on) {
    sprintf(temp, "/jump %s %d",
            (ship->hyper_drive.ready ? "ready" : "charging"),
            ship->hyper_drive.charge);
    strcat(buf, temp);
  }
  if (ship->protect.self) {
    sprintf(temp, "/retal");
    strcat(buf, temp);
  }

  if (ship->guns == PRIMARY) {
    switch (ship->primtype) {
      case GTYPE_LIGHT:
        sprintf(temp, "/lgt primary");
        break;
      case GTYPE_MEDIUM:
        sprintf(temp, "/med primary");
        break;
      case GTYPE_HEAVY:
        sprintf(temp, "/hvy primary");
        break;
      case GTYPE_NONE:
        sprintf(temp, "/none");
        break;
    }
    strcat(buf, temp);
  } else if (ship->guns == SECONDARY) {
    switch (ship->sectype) {
      case GTYPE_LIGHT:
        sprintf(temp, "/lgt secondary");
        break;
      case GTYPE_MEDIUM:
        sprintf(temp, "/med secndry");
        break;
      case GTYPE_HEAVY:
        sprintf(temp, "/hvy secndry");
        break;
      case GTYPE_NONE:
        sprintf(temp, "/none");
        break;
    }
    strcat(buf, temp);
  }

  if (ship->fire_laser) {
    sprintf(temp, "/laser %d", ship->fire_laser);
    strcat(buf, temp);
  }
  if (ship->focus) strcat(buf, "/focus");

  if (ship->retaliate) {
    sprintf(temp, "/salvo %d", ship->retaliate);
    strcat(buf, temp);
  }
  if (ship->protect.planet) strcat(buf, "/defense");
  if (ship->protect.on) {
    sprintf(temp, "/prot %d", ship->protect.ship);
    strcat(buf, temp);
  }
  if (ship->navigate.on) {
    sprintf(temp, "/nav %d (%d)", ship->navigate.bearing, ship->navigate.turns);
    strcat(buf, temp);
  }
  if (ship->merchant) {
    sprintf(temp, "/merchant %d", ship->merchant);
    strcat(buf, temp);
  }
  if (has_switch(ship)) {
    if (ship->on)
      strcat(buf, "/on");
    else
      strcat(buf, "/off");
  }
  if (ship->protect.evade) strcat(buf, "/evade");
  if (ship->bombard) strcat(buf, "/bomb");
  if (ship->type == STYPE_MINE || ship->type == OTYPE_GR) {
    if (ship->mode)
      strcat(buf, "/radiate");
    else
      strcat(buf, "/explode");
  }
  if (ship->type == OTYPE_TERRA || ship->type == OTYPE_PLOW) {
    int i;
    sprintf(temp, "/move %s",
            &(ship->shipclass[ship->special.terraform.index]));
    if (temp[i = (strlen(temp) - 1)] == 'c') {
      char c = ship->shipclass[ship->special.terraform.index];
      ship->shipclass[ship->special.terraform.index] = '\0';
      sprintf(temp + i, "%sc", ship->shipclass);
      ship->shipclass[ship->special.terraform.index] = c;
    }
    strcat(buf, temp);
  }

  if (ship->type == STYPE_MISSILE && ship->whatdest == ScopeLevel::LEVEL_PLAN) {
    if (ship->special.impact.scatter)
      strcat(buf, "/scatter");
    else {
      sprintf(temp, "/impact %d,%d", ship->special.impact.x,
              ship->special.impact.y);
      strcat(buf, temp);
    }
  }

  if (ship->type == STYPE_MINE) {
    sprintf(temp, "/trigger %d", ship->special.trigger.radius);
    strcat(buf, temp);
  }
  if (ship->type == OTYPE_TRANSDEV) {
    sprintf(temp, "/target %d", ship->special.transport.target);
    strcat(buf, temp);
  }
  if (ship->type == STYPE_MIRROR) {
    sprintf(temp, "/aim %s/int %d", prin_aimed_at(*ship).c_str(),
            ship->special.aimed_at.intensity);
    strcat(buf, temp);
  }

  strcat(buf, "\n");
  notify(Playernum, Governor, buf);
  /* if hyper space is on estimate how much fuel it will cost to get to the
   * destination */
  if (ship->hyper_drive.on) {
    double dist, fuse;

    dist = sqrt(Distsq(ship->xpos, ship->ypos, Stars[ship->deststar]->xpos,
                       Stars[ship->deststar]->ypos));
    distfac = HYPER_DIST_FACTOR * (ship->tech + 100.0);
    if (ship->mounted && dist > distfac) {
      fuse = HYPER_DRIVE_FUEL_USE * sqrt(ship->mass) * (dist / distfac);
    } else {
      fuse = HYPER_DRIVE_FUEL_USE * sqrt(ship->mass) * (dist / distfac) *
             (dist / distfac);
    }

    sprintf(buf, "  *** distance %.0f - jump will cost %.1ff ***\n", dist,
            fuse);
    notify(Playernum, Governor, buf);
    if (ship->max_fuel < fuse)
      notify(Playernum, Governor,
             "Your ship cannot carry enough fuel to do this jump.\n");
  }
}

void route(const command_t &argv, GameObj &g) {
  // TODO(jeffbailey): This seems to segfault with no args.
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  int i, x, y;
  unsigned char star, planet, load, unload;
  const char *c;
  placetype where;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You have to 'cs' to a planet to examine routes.\n");
    return;
  }
  auto p = getplanet(g.snum, g.pnum);
  if (argv.size() == 1) { /* display all shipping routes that are active */
    for (i = 1; i <= MAX_ROUTES; i++)
      if (p.info[Playernum - 1].route[i - 1].set) {
        star = p.info[Playernum - 1].route[i - 1].dest_star;
        planet = p.info[Playernum - 1].route[i - 1].dest_planet;
        load = p.info[Playernum - 1].route[i - 1].load;
        unload = p.info[Playernum - 1].route[i - 1].unload;
        sprintf(buf, "%2d  land %2d,%2d   ", i,
                p.info[Playernum - 1].route[i - 1].x,
                p.info[Playernum - 1].route[i - 1].y);
        strcat(buf, "load: ");
        if (Fuel(load))
          strcat(buf, "f");
        else
          strcat(buf, " ");
        if (Destruct(load))
          strcat(buf, "d");
        else
          strcat(buf, " ");
        if (Resources(load))
          strcat(buf, "r");
        else
          strcat(buf, " ");
        if (Crystals(load)) strcat(buf, "x");
        strcat(buf, " ");

        strcat(buf, "  unload: ");
        if (Fuel(unload))
          strcat(buf, "f");
        else
          strcat(buf, " ");
        if (Destruct(unload))
          strcat(buf, "d");
        else
          strcat(buf, " ");
        if (Resources(unload))
          strcat(buf, "r");
        else
          strcat(buf, " ");
        if (Crystals(unload))
          strcat(buf, "x");
        else
          strcat(buf, " ");
        sprintf(temp, "  -> %s/%s\n", Stars[star]->name,
                Stars[star]->pnames[planet]);
        strcat(buf, temp);
        notify(Playernum, Governor, buf);
      }
    g.out << "Done.\n";
    return;
  } else if (argv.size() == 2) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (p.info[Playernum - 1].route[i - 1].set) {
      star = p.info[Playernum - 1].route[i - 1].dest_star;
      planet = p.info[Playernum - 1].route[i - 1].dest_planet;
      load = p.info[Playernum - 1].route[i - 1].load;
      unload = p.info[Playernum - 1].route[i - 1].unload;
      sprintf(buf, "%2d  land %2d,%2d   ", i,
              p.info[Playernum - 1].route[i - 1].x,
              p.info[Playernum - 1].route[i - 1].y);
      if (load) {
        sprintf(temp, "load: ");
        strcat(buf, temp);
        if (Fuel(load)) strcat(buf, "f");
        if (Destruct(load)) strcat(buf, "d");
        if (Resources(load)) strcat(buf, "r");
        if (Crystals(load)) strcat(buf, "x");
      }
      if (unload) {
        sprintf(temp, "  unload: ");
        strcat(buf, temp);
        if (Fuel(unload)) strcat(buf, "f");
        if (Destruct(unload)) strcat(buf, "d");
        if (Resources(unload)) strcat(buf, "r");
        if (Crystals(unload)) strcat(buf, "x");
      }
      sprintf(temp, "  ->  %s/%s\n", Stars[star]->name,
              Stars[star]->pnames[planet]);
      strcat(buf, temp);
      notify(Playernum, Governor, buf);
    }
    g.out << "Done.\n";
    return;
  } else if (argv.size() == 3) {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (argv[2] == "activate")
      p.info[Playernum - 1].route[i - 1].set = 1;
    else if (argv[2] == "deactivate")
      p.info[Playernum - 1].route[i - 1].set = 0;
    else {
      where = Getplace(g, argv[2].c_str(), 1);
      if (!where.err) {
        if (where.level != ScopeLevel::LEVEL_PLAN) {
          g.out << "You have to designate a planet.\n";
          return;
        }
        p.info[Playernum - 1].route[i - 1].dest_star = where.snum;
        p.info[Playernum - 1].route[i - 1].dest_planet = where.pnum;
        g.out << "Set.\n";
      } else {
        g.out << "Illegal destination.\n";
        return;
      }
    }
  } else {
    i = std::stoi(argv[1]);
    if (i > MAX_ROUTES || i < 1) {
      g.out << "Bad route number.\n";
      return;
    }
    if (argv[2] == "land") {
      sscanf(argv[3].c_str(), "%d,%d", &x, &y);
      if (x < 0 || x > p.Maxx - 1 || y < 0 || y > p.Maxy - 1) {
        g.out << "Bad sector coordinates.\n";
        return;
      }
      p.info[Playernum - 1].route[i - 1].x = x;
      p.info[Playernum - 1].route[i - 1].y = y;
    } else if (argv[2] == "load") {
      p.info[Playernum - 1].route[i - 1].load = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info[Playernum - 1].route[i - 1].load |= M_FUEL;
        if (*c == 'd') p.info[Playernum - 1].route[i - 1].load |= M_DESTRUCT;
        if (*c == 'r') p.info[Playernum - 1].route[i - 1].load |= M_RESOURCES;
        if (*c == 'x') p.info[Playernum - 1].route[i - 1].load |= M_CRYSTALS;
        c++;
      }
    } else if (argv[2] == "unload") {
      p.info[Playernum - 1].route[i - 1].unload = 0;
      c = argv[3].c_str();
      while (*c) {
        if (*c == 'f') p.info[Playernum - 1].route[i - 1].unload |= M_FUEL;
        if (*c == 'd') p.info[Playernum - 1].route[i - 1].unload |= M_DESTRUCT;
        if (*c == 'r') p.info[Playernum - 1].route[i - 1].unload |= M_RESOURCES;
        if (*c == 'x') p.info[Playernum - 1].route[i - 1].unload |= M_CRYSTALS;
        c++;
      }
    } else {
      g.out << "What are you trying to do?\n";
      return;
    }
    g.out << "Set.\n";
  }
  putplanet(p, Stars[g.snum], g.pnum);
}
