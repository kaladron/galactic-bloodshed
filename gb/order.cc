// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  order.c -- give orders to ship */

import gblib;
import std.compat;

#include "gb/buffers.h"

static std::string prin_aimed_at(const Ship &);
static void mk_expl_aimed_at(player_t, governor_t, Ship *);

// TODO(jeffbailey): We take in a non-zero APcount, and do nothing with it!
void give_orders(GameObj &g, const command_t &argv, int /* APcount */,
                 Ship *ship) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int j;

  if (!ship->active) {
    sprintf(buf, "%s is irradiated (%d); it cannot be given orders.\n",
            ship_to_string(*ship).c_str(), ship->rad);
    notify(Playernum, Governor, buf);
    return;
  }
  if (ship->type != ShipType::OTYPE_TRANSDEV && !ship->popn &&
      max_crew(*ship)) {
    sprintf(buf, "%s has no crew and is not a robotic ship.\n",
            ship_to_string(*ship).c_str());
    notify(Playernum, Governor, buf);
    return;
  }

  if (argv[2] == "defense") {
    if (can_bombard(*ship)) {
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
    if (ship->type != ShipType::STYPE_MISSILE) {
      g.out << "Only missiles can be given this order.\n";
      return;
    }
    ship->special.impact.scatter = 1;
  } else if (argv[2] == "impact") {
    int x;
    int y;
    if (ship->type != ShipType::STYPE_MISSILE) {
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
    if (can_bombard(*ship)) {
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
      ship->navigate.bearing = std::stoi(argv[3]);
      ship->navigate.turns = std::stoi(argv[4]);
    } else
      ship->navigate.on = 0;
    if (ship->hyper_drive.on) ship->hyper_drive.on = 0;
  } else if (argv[2] == "switch") {
    if (ship->type == ShipType::OTYPE_FACTORY) {
      g.out << "Use \"on\" to bring factory online.\n";
      return;
    }
    if (has_switch(*ship)) {
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
        case ShipType::STYPE_MINE:
          g.out << "Mine armed and ready.\n";
          break;
        case ShipType::OTYPE_TRANSDEV:
          g.out << "Transporter ready to receive.\n";
          break;
        default:
          break;
      }
    } else {
      switch (ship->type) {
        case ShipType::STYPE_MINE:
          g.out << "Mine disarmed.\n";
          break;
        case ShipType::OTYPE_TRANSDEV:
          g.out << "No longer receiving.\n";
          break;
        default:
          break;
      }
    }
  } else if (argv[2] == "destination") {
    if (speed_rating(*ship)) {
      if (ship->docked) {
        notify(Playernum, Governor,
               "That ship is docked; use undock or launch first.\n");
        return;
      }
      Place where{g, argv[3], true};
      if (!where.err) {
        if (where.level == ScopeLevel::LEVEL_SHIP) {
          auto tmpship = getship(where.shipno);
          if (!followable(*ship, *tmpship)) {
            g.out << "Warning: that ship is out of range.\n";
            return;
          }
          ship->destshipno = where.shipno;
          ship->whatdest = ScopeLevel::LEVEL_SHIP;
        } else {
          /* to foil cheaters */
          if (where.level != ScopeLevel::LEVEL_UNIV &&
              ((ship->storbits != where.snum) &&
               where.level != ScopeLevel::LEVEL_STAR) &&
              isclr(stars[where.snum].explored, ship->owner)) {
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
    if (max_crew(*ship) && max_speed(*ship)) {
      if (argv[3] == "on")
        ship->protect.evade = 1;
      else if (argv[3] == "off")
        ship->protect.evade = 0;
    } else
      return;
  } else if (argv[2] == "bombard") {
    if (ship->type != ShipType::OTYPE_OMCL) {
      if (can_bombard(*ship)) {
        if (argv[3] == "off")
          ship->bombard = 0;
        else if (argv[3] == "on")
          ship->bombard = 1;
      } else
        notify(Playernum, Governor,
               "This type of ship cannot be set to retaliate.\n");
    }
  } else if (argv[2] == "retaliate") {
    if (ship->type != ShipType::OTYPE_OMCL) {
      if (can_bombard(*ship)) {
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
      if (can_bombard(*ship)) {
        if (ship->mounted) {
          if (argv[3] == "on")
            ship->fire_laser = std::stoi(argv[4]);
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
      j = std::stoi(argv[3]);
      if (j < 0 || j > MAX_ROUTES) {
        g.out << "Bad route number.\n";
        return;
      }
      ship->merchant = j;
    }
  } else if (argv[2] == "speed") {
    if (speed_rating(*ship)) {
      j = std::stoi(argv[3]);
      if (j < 0) {
        g.out << "Specify a positive speed.\n";
        return;
      }
      if (j > speed_rating(*ship)) j = speed_rating(*ship);
      ship->speed = j;

    } else {
      g.out << "This ship does not have a speed rating.\n";
      return;
    }
  } else if (argv[2] == "salvo") {
    if (can_bombard(*ship)) {
      j = std::stoi(argv[3]);
      if (j < 0) {
        g.out << "Specify a positive number of guns.\n";
        return;
      }
      if (ship->guns == PRIMARY && j > ship->primary)
        j = ship->primary;
      else if (ship->guns == SECONDARY && j > ship->secondary)
        j = ship->secondary;
      else if (ship->guns == GTYPE_NONE)
        j = 0;

      ship->retaliate = j;

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
        j = std::stoi(argv[3]);
        if (j < 0) {
          notify(Playernum, Governor,
                 "Specify a nonnegative number of guns.\n");
          return;
        }
        if (j > ship->primary) j = ship->primary;
        ship->retaliate = j;
        ship->guns = PRIMARY;
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
        j = std::stoi(argv[3]);
        if (j < 0) {
          notify(Playernum, Governor,
                 "Specify a nonnegative number of guns.\n");
          return;
        }
        if (j > ship->secondary) j = ship->secondary;
        ship->retaliate = j;
        ship->guns = SECONDARY;
      }
    } else {
      g.out << "This ship does not have secondary guns.\n";
      return;
    }
  } else if (argv[2] == "explosive") {
    switch (ship->type) {
      case ShipType::STYPE_MINE:
      case ShipType::OTYPE_GR:
        ship->mode = 0;
        break;
      default:
        return;
    }
  } else if (argv[2] == "radiative") {
    switch (ship->type) {
      case ShipType::STYPE_MINE:
      case ShipType::OTYPE_GR:
        ship->mode = 1;
        break;
      default:
        return;
    }
  } else if (argv[2] == "move") {
    if ((ship->type != ShipType::OTYPE_TERRA) &&
        (ship->type != ShipType::OTYPE_PLOW)) {
      g.out << "That ship is not a terraformer or a space plow.\n";
      return;
    }
    std::string moveseq;
    if (argv.size() > 3) {
      moveseq = argv[3];
    } else { /* The move list might be empty.. */
      moveseq = "5";
    }
    for (auto i = 0; i < moveseq.size(); ++i) {
      /* Make sure the list of moves is short enough. */
      if (i == SHIP_NAMESIZE - 1) {
        sprintf(buf, "Warning: that is more than %d moves.\n",
                SHIP_NAMESIZE - 1);
        notify(Playernum, Governor, buf);
        g.out << "These move orders have been truncated.\n";
        moveseq.resize(i);
        break;
      }
      /* Make sure this move is OK. */
      if ((moveseq[i] == 'c') || (moveseq[i] == 's')) {
        if ((i == 0) && (moveseq[0] == 'c')) {
          g.out << "Cycling move orders can not be empty!\n";
          return;
        }
        if (moveseq[i + 1]) {
          sprintf(buf,
                  "Warning: '%c' should be the last character in the "
                  "move order.\n",
                  moveseq[i]);
          notify(Playernum, Governor, buf);
          g.out << "These move orders have been truncated.\n";
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
  } else if (argv[2] == "trigger") {
    if (ship->type == ShipType::STYPE_MINE) {
      if (std::stoi(argv[3]) < 0)
        ship->special.trigger.radius = 0;
      else
        ship->special.trigger.radius = std::stoi(argv[3]);
    } else {
      notify(Playernum, Governor,
             "This ship cannot be assigned a trigger radius.\n");
      return;
    }
  } else if (argv[2] == "transport") {
    if (ship->type == ShipType::OTYPE_TRANSDEV) {
      ship->special.transport.target = std::stoi(argv[3]);
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
    if (can_aim(*ship)) {
      if (ship->type == ShipType::OTYPE_GTELE ||
          ship->type == ShipType::OTYPE_TRACT || ship->fuel >= FUEL_MANEUVER) {
        if (ship->type == ShipType::STYPE_MIRROR && ship->docked) {
          sprintf(buf, "docked; use undock or launch first.\n");
          notify(Playernum, Governor, buf);
          return;
        }
        Place pl{g, argv[3], true};
        if (pl.err) {
          g.out << "Error in destination.\n";
          return;
        }
        ship->special.aimed_at.level = pl.level;
        ship->special.aimed_at.pnum = pl.pnum;
        ship->special.aimed_at.snum = pl.snum;
        ship->special.aimed_at.shipno = pl.shipno;
        if (ship->type != ShipType::OTYPE_TRACT &&
            ship->type != ShipType::OTYPE_GTELE)
          use_fuel(*ship, FUEL_MANEUVER);
        if (ship->type == ShipType::OTYPE_GTELE ||
            ship->type == ShipType::OTYPE_STELE)
          mk_expl_aimed_at(Playernum, Governor, ship);
        sprintf(buf, "Aimed at %s\n", prin_aimed_at(*ship).c_str());
        notify(Playernum, Governor, buf);

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
    if (ship->type == ShipType::STYPE_MIRROR) {
      ship->special.aimed_at.intensity =
          std::max(0, std::min(100, std::stoi(argv[3])));
    }
  } else if (argv[2] == "on") {
    if (!has_switch(*ship)) {
      notify(Playernum, Governor,
             "This ship does not have an on/off setting.\n");
      return;
    }
    if (ship->damage && ship->type != ShipType::OTYPE_FACTORY) {
      g.out << "Damaged ships cannot be activated.\n";
      return;
    }
    if (ship->on) {
      g.out << "This ship is already activated.\n";
      return;
    }
    if (ship->type == ShipType::OTYPE_FACTORY) {
      unsigned int oncost;
      if (ship->whatorbits == ScopeLevel::LEVEL_SHIP) {
        int hangerneeded;

        auto s2 = getship(ship->destshipno);
        if (s2->type == ShipType::STYPE_HABITAT) {
          oncost = HAB_FACT_ON_COST * ship->build_cost;
          if (s2->resource < oncost) {
            sprintf(buf,
                    "You don't have %d resources on Habitat #%lu to "
                    "activate this factory.\n",
                    oncost, ship->destshipno);
            notify(Playernum, Governor, buf);
            return;
          }
          hangerneeded = (1 + (int)(HAB_FACT_SIZE * (double)ship_size(*ship))) -
                         ((s2->max_hanger - s2->hanger) + ship->size);
          if (hangerneeded > 0) {
            sprintf(
                buf,
                "Not enough hanger space free on Habitat #%lu. Need %d more.\n",
                ship->destshipno, hangerneeded);
            notify(Playernum, Governor, buf);
            return;
          }
          s2->resource -= oncost;
          s2->hanger -= ship->size;
          ship->size = 1 + (int)(HAB_FACT_SIZE * (double)ship_size(*ship));
          s2->hanger += ship->size;
          putship(&*s2);
        } else {
          g.out << "The factory is currently being transported.\n";
          return;
        }
      } else if (!landed(*ship)) {
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
        }
        planet.info[Playernum - 1].resource -= oncost;
        putplanet(planet, stars[ship->deststar], (int)ship->destpnum);
      }
      sprintf(buf, "Factory activated at a cost of %d resources.\n", oncost);
      notify(Playernum, Governor, buf);
    }
    ship->on = 1;
  } else if (argv[2] == "off") {
    if (ship->type == ShipType::OTYPE_FACTORY && ship->on) {
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
  Place targ{ship.special.aimed_at.level, ship.special.aimed_at.snum,
             ship.special.aimed_at.pnum, ship.special.aimed_at.shipno};
  return targ.to_string();
}

/*
 * mark wherever the ship is aimed at, as explored by the owning player.
 */
static void mk_expl_aimed_at(player_t Playernum, governor_t Governor, Ship *s) {
  double dist;
  double xf;
  double yf;

  auto &str = stars[s->special.aimed_at.snum];

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
      if ((dist = sqrt(Distsq(xf, yf, str.xpos, str.ypos))) <=
          tele_range((int)s->type, s->tech)) {
        str = getstar(s->special.aimed_at.snum);
        setbit(str.explored, Playernum);
        putstar(str, s->special.aimed_at.snum);
        sprintf(buf, "Surveyed, distance %g.\n", dist);
        notify(Playernum, Governor, buf);
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
      if ((dist = sqrt(Distsq(xf, yf, str.xpos + p.xpos, str.ypos + p.ypos))) <=
          tele_range((int)s->type, s->tech)) {
        setbit(str.explored, Playernum);
        p.info[Playernum - 1].explored = 1;
        putplanet(p, stars[s->special.aimed_at.snum], s->special.aimed_at.pnum);
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

void DispOrdersHeader(int Playernum, int Governor) {
  notify(Playernum, Governor,
         "    #       name       sp orbits     destin     options\n");
}

void DispOrders(int Playernum, int Governor, const Ship &ship) {
  double distfac;
  std::stringstream buffer;

  if (ship.owner != Playernum || !authorized(Governor, ship) || !ship.alive)
    return;

  if (ship.docked)
    if (ship.whatdest == ScopeLevel::LEVEL_SHIP)
      buffer << "D#" << ship.destshipno;
    else
      buffer << std::format("L{:2d},{:2d}", ship.land_x, ship.land_y);
  else
    buffer << prin_ship_dest(ship);

  buffer << std::format("{:5} {} {:14.14} {}{} {:10.10} {}", ship.number,
                        Shipltrs[ship.type], ship.name,
                        ship.hyper_drive.has ? (ship.mounted ? '+' : '*') : ' ',
                        ship.speed, dispshiploc_brief(ship), buffer.str());

  if (ship.hyper_drive.on) {
    buffer << std::format("/jump {} {}",
                          (ship.hyper_drive.ready ? "ready" : "charging"),
                          ship.hyper_drive.charge);
  }
  if (ship.protect.self) {
    buffer << "/retal";
  }

  if (ship.guns == PRIMARY) {
    switch (ship.primtype) {
      case GTYPE_LIGHT:
        buffer << "/lgt primary";
        break;
      case GTYPE_MEDIUM:
        buffer << "/med primary";
        break;
      case GTYPE_HEAVY:
        buffer << "/hvy primary";
        break;
      case GTYPE_NONE:
        buffer << "/none";
        break;
    }
  } else if (ship.guns == SECONDARY) {
    switch (ship.sectype) {
      case GTYPE_LIGHT:
        buffer << "/lgt secondary";
        break;
      case GTYPE_MEDIUM:
        buffer << "/med secndry";
        break;
      case GTYPE_HEAVY:
        buffer << "/hvy secndry";
        break;
      case GTYPE_NONE:
        buffer << "/none";
        break;
    }
  }

  if (ship.fire_laser) {
    buffer << std::format("/laser {}", ship.fire_laser);
  }
  if (ship.focus) buffer << "/focus";

  if (ship.retaliate) {
    buffer << std::format("/salvo {}", ship.retaliate);
  }
  if (ship.protect.planet) buffer << "/defense";
  if (ship.protect.on) {
    buffer << std::format("/prot {}", ship.protect.ship);
  }
  if (ship.navigate.on) {
    buffer << std::format("/nav {} ({})", ship.navigate.bearing,
                          ship.navigate.turns);
  }
  if (ship.merchant) {
    buffer << std::format("/merchant {}", ship.merchant);
  }
  if (has_switch(ship)) {
    if (ship.on)
      buffer << "/on";
    else
      buffer << "/off";
  }
  if (ship.protect.evade) buffer << "/evade";
  if (ship.bombard) buffer << "/bomb";
  if (ship.type == ShipType::STYPE_MINE || ship.type == ShipType::OTYPE_GR) {
    if (ship.mode)
      buffer << "/radiate";
    else
      buffer << "/explode";
  }
  if (ship.type == ShipType::OTYPE_TERRA || ship.type == ShipType::OTYPE_PLOW) {
    int i;
    std::string temp = &(ship.shipclass[ship.special.terraform.index]);
    buffer << std::format("/move {}", temp);

    if (temp[i = (temp.length() - 1)] == 'c') {
      std::string hidden = temp;
      hidden = hidden.substr(0, ship.special.terraform.index);
      buffer << std::format("{}c", hidden);
    }
  }

  if (ship.type == ShipType::STYPE_MISSILE &&
      ship.whatdest == ScopeLevel::LEVEL_PLAN) {
    if (ship.special.impact.scatter)
      buffer << "/scatter";
    else {
      buffer << std::format("/impact {},{}", ship.special.impact.x,
                            ship.special.impact.y);
    }
  }

  if (ship.type == ShipType::STYPE_MINE) {
    buffer << std::format("/trigger {}", ship.special.trigger.radius);
  }
  if (ship.type == ShipType::OTYPE_TRANSDEV) {
    buffer << std::format("/target {}", ship.special.transport.target);
  }
  if (ship.type == ShipType::STYPE_MIRROR) {
    buffer << std::format("/aim {}/int {}", prin_aimed_at(ship),
                          ship.special.aimed_at.intensity);
  }

  buffer << "\n";
  notify(Playernum, Governor, buffer.str());
  /* if hyper space is on estimate how much fuel it will cost to get to the
   * destination */
  if (ship.hyper_drive.on) {
    double dist;
    double fuse;

    dist = sqrt(Distsq(ship.xpos, ship.ypos, stars[ship.deststar].xpos,
                       stars[ship.deststar].ypos));
    distfac = HYPER_DIST_FACTOR * (ship.tech + 100.0);
    if (ship.mounted && dist > distfac) {
      fuse = HYPER_DRIVE_FUEL_USE * sqrt(ship.mass) * (dist / distfac);
    } else {
      fuse = HYPER_DRIVE_FUEL_USE * sqrt(ship.mass) * (dist / distfac) *
             (dist / distfac);
    }

    notify(Playernum, Governor,
           std::format("  *** distance {:.0f} - jump will cost {:.1f}f ***\n",
                       dist, fuse));
    if (ship.max_fuel < fuse)
      notify(Playernum, Governor,
             "Your ship cannot carry enough fuel to do this jump.\n");
  }
}
