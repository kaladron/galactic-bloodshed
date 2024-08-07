// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  moveship -- moves specified ship according to its orders.
 *	also deducts fuel from the ship's stores. */

import gblib;
import std.compat;

#include "gb/moveship.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/fire.h"
#include "gb/load.h"
#include "gb/max.h"
#include "gb/order.h"
#include "gb/races.h"
#include "gb/tele.h"
#include "gb/tweakables.h"

/* amount to move for each dir level. I arrived on these #'s only after
        hours of dilligent tweaking */
/* amount to move for each directory level  */
static const double MoveConsts[] = {600.0, 300.0, 50.0};
/* amnt to move for each ship speed level (ordered) */
static const double SpeedConsts[] = {0.0,  0.61, 1.26, 1.50, 1.73,
                                     1.81, 1.90, 1.93, 1.96, 1.97};
/* amount of fuel it costs to move at speed level */

static int do_merchant(Ship *, Planet &);

void moveship(Ship *s, int mode, int send_messages, int checking_fuel) {
  double stardist;
  double movedist;
  double truedist;
  double dist;
  double xdest;
  double ydest;
  double sn;
  double cs;
  double mfactor;
  double heading;
  double distfac;
  double fuse;
  ScopeLevel destlevel;
  int deststar = 0;
  int destpnum = 0;
  Ship *dsh;

  if (s->hyper_drive.has && s->hyper_drive.on) { /* do a hyperspace jump */
    if (!mode) return; /* we're not ready to jump until the update */
    if (s->hyper_drive.ready) {
      dist = sqrt(Distsq(s->xpos, s->ypos, stars[s->deststar].xpos,
                         stars[s->deststar].ypos));
      distfac = HYPER_DIST_FACTOR * (s->tech + 100.0);
      if (s->mounted && dist > distfac)
        fuse = HYPER_DRIVE_FUEL_USE * sqrt(s->mass) * (dist / distfac);
      else
        fuse = HYPER_DRIVE_FUEL_USE * sqrt(s->mass) * (dist / distfac) *
               (dist / distfac);

      if (s->fuel < fuse) {
        sprintf(telegram_buf,
                "%s at system %s does not have %.1ff to do hyperspace jump.",
                ship_to_string(*s).c_str(), prin_ship_orbits(*s).c_str(), fuse);
        if (send_messages)
          push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
        s->hyper_drive.on = 0;
        return;
      }
      use_fuel(*s, fuse);
      heading = atan2(stars[s->deststar].xpos - s->xpos,
                      stars[s->deststar].ypos - s->ypos);
      sn = sin(heading);
      cs = cos(heading);
      s->xpos = stars[s->deststar].xpos - sn * 0.9 * SYSTEMSIZE;
      s->ypos = stars[s->deststar].ypos - cs * 0.9 * SYSTEMSIZE;
      s->whatorbits = ScopeLevel::LEVEL_STAR;
      s->storbits = s->deststar;
      s->protect.planet = 0;
      s->hyper_drive.on = 0;
      s->hyper_drive.ready = 0;
      s->hyper_drive.charge = 0;
      sprintf(telegram_buf, "%s arrived at %s.", ship_to_string(*s).c_str(),
              prin_ship_orbits(*s).c_str());
      if (send_messages)
        push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
    } else if (s->mounted) {
      s->hyper_drive.ready = 1;
      s->hyper_drive.charge = HYPER_DRIVE_READY_CHARGE;
    } else {
      if (s->hyper_drive.charge == HYPER_DRIVE_READY_CHARGE)
        s->hyper_drive.ready = 1;
      else
        s->hyper_drive.charge += 1;
    }
    return;
  }
  if (s->speed && !s->docked && s->alive &&
      (s->whatdest != ScopeLevel::LEVEL_UNIV || s->navigate.on)) {
    fuse = 0.5 * s->speed * (1 + s->protect.evade) * s->mass * FUEL_USE /
           (double)segments;
    if (s->fuel < fuse) {
      if (send_messages) msg_OOF(s); /* send OOF notify */
      if (s->whatorbits == ScopeLevel::LEVEL_UNIV &&
          (s->build_cost <= 50 || s->type == ShipType::OTYPE_VN ||
           s->type == ShipType::OTYPE_BERS)) {
        sprintf(telegram_buf, "%s has been lost in deep space.",
                ship_to_string(*s).c_str());
        if (send_messages)
          push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
        if (send_messages) kill_ship((int)(s->owner), s);
      }
      return;
    }
    if (s->navigate.on) { /* follow navigational orders */
      heading = .0174329252 * s->navigate.bearing;
      mfactor = SHIP_MOVE_SCALE * (1.0 - .01 * s->rad) *
                (1.0 - .01 * s->damage) * SpeedConsts[s->speed] *
                MoveConsts[s->whatorbits] / (double)segments;
      use_fuel(*s, (double)fuse);
      sn = sin(heading);
      cs = cos(heading);
      xdest = sn * mfactor;
      ydest = -cs * mfactor;
      s->xpos += xdest;
      s->ypos += ydest;
      s->navigate.turns--;
      if (!s->navigate.turns) s->navigate.on = 0;
      /* check here for orbit breaking as well. Maarten */
      auto &ost = stars[s->storbits];
      const auto &opl = planets[s->storbits][s->pnumorbits];
      if (s->whatorbits == ScopeLevel::LEVEL_PLAN) {
        dist = sqrt(Distsq(s->xpos, s->ypos, ost.xpos + opl->xpos,
                           ost.ypos + opl->ypos));
        if (dist > PLORBITSIZE) {
          s->whatorbits = ScopeLevel::LEVEL_STAR;
          s->protect.planet = 0;
        }
      } else if (s->whatorbits == ScopeLevel::LEVEL_STAR) {
        dist = sqrt(Distsq(s->xpos, s->ypos, ost.xpos, ost.ypos));
        if (dist > SYSTEMSIZE) {
          s->whatorbits = ScopeLevel::LEVEL_UNIV;
          s->protect.evade = 0;
          s->protect.planet = 0;
        }
      }
    } else { /*		navigate is off            */
      destlevel = s->whatdest;
      if (destlevel == ScopeLevel::LEVEL_SHIP) {
        dsh = ships[s->destshipno];
        s->deststar = dsh->storbits;
        s->destpnum = dsh->pnumorbits;
        xdest = dsh->xpos;
        ydest = dsh->ypos;
        switch (dsh->whatorbits) {
          case ScopeLevel::LEVEL_UNIV:
            break;
          case ScopeLevel::LEVEL_PLAN:
            if (s->whatorbits != dsh->whatorbits ||
                s->pnumorbits != dsh->pnumorbits)
              destlevel = ScopeLevel::LEVEL_PLAN;
            break;
          case ScopeLevel::LEVEL_STAR:
            if (s->whatorbits != dsh->whatorbits ||
                s->storbits != dsh->storbits)
              destlevel = ScopeLevel::LEVEL_STAR;
            break;
          case ScopeLevel::LEVEL_SHIP:
            // TODO(jeffbailey): Prove that this is impossible.
            break;
        }
        /*			if (sqrt( (double)Distsq(s->xpos, s->ypos,
           xdest,
           ydest))
                   <= DIST_TO_LAND || !(dsh->alive)) {
                           destlevel = ScopeLevel::LEVEL_UNIV;
                                                   s->whatdest=ScopeLevel::LEVEL_UNIV;
                                   } */
      }
      /*		else */
      if (destlevel == ScopeLevel::LEVEL_STAR ||
          (destlevel == ScopeLevel::LEVEL_PLAN &&
           (s->storbits != s->deststar ||
            s->whatorbits == ScopeLevel::LEVEL_UNIV))) {
        destlevel = ScopeLevel::LEVEL_STAR;
        deststar = s->deststar;
        xdest = stars[deststar].xpos;
        ydest = stars[deststar].ypos;
      } else if (destlevel == ScopeLevel::LEVEL_PLAN &&
                 s->storbits == s->deststar) {
        destlevel = ScopeLevel::LEVEL_PLAN;
        deststar = s->deststar;
        destpnum = s->destpnum;
        xdest = stars[deststar].xpos + planets[deststar][destpnum]->xpos;
        ydest = stars[deststar].ypos + planets[deststar][destpnum]->ypos;
        if (sqrt(Distsq(s->xpos, s->ypos, xdest, ydest)) <= DIST_TO_LAND)
          destlevel = ScopeLevel::LEVEL_UNIV;
      }
      auto &dst = stars[deststar];
      auto &ost = stars[s->storbits];
      const auto &dpl = planets[deststar][destpnum];
      const auto &opl = planets[s->storbits][s->pnumorbits];
      truedist = movedist = sqrt(Distsq(s->xpos, s->ypos, xdest, ydest));
      /* Save some unneccesary calculation and domain errors for atan2
            Maarten */
      if (truedist < DIST_TO_LAND && s->whatorbits == destlevel &&
          s->storbits == deststar && s->pnumorbits == destpnum)
        return;
      heading = atan2((double)(xdest - s->xpos), (double)(-ydest + s->ypos));
      mfactor = SHIP_MOVE_SCALE * (1. - .01 * (double)s->rad) *
                (1. - .01 * (double)s->damage) * SpeedConsts[s->speed] *
                MoveConsts[s->whatorbits] / (double)segments;

      /* keep from ending up in the middle of the system. */
      if (destlevel == ScopeLevel::LEVEL_STAR &&
          (s->storbits != deststar || s->whatorbits == ScopeLevel::LEVEL_UNIV))
        movedist -= SYSTEMSIZE * 0.90;
      else if (destlevel == ScopeLevel::LEVEL_PLAN &&
               s->whatorbits == ScopeLevel::LEVEL_STAR &&
               s->storbits == deststar && truedist >= PLORBITSIZE)
        movedist -= PLORBITSIZE * 0.90;

      if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
          !followable(s, ships[s->destshipno])) {
        s->whatdest = ScopeLevel::LEVEL_UNIV;
        s->protect.evade = 0;
        sprintf(telegram_buf, "%s at %s lost sight of destination ship #%ld.",
                ship_to_string(*s).c_str(), prin_ship_orbits(*s).c_str(),
                s->destshipno);
        if (send_messages)
          push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
        return;
      }
      if (truedist > DIST_TO_LAND) {
        use_fuel(*s, (double)fuse);
        /* dont overshoot */
        sn = sin(heading);
        cs = cos(heading);
        xdest = sn * mfactor;
        ydest = -cs * mfactor;
        if (hypot(xdest, ydest) > movedist) {
          xdest = sn * movedist;
          ydest = -cs * movedist;
        }
        s->xpos += xdest;
        s->ypos += ydest;
      }
      /***** check if far enough away from object it's orbiting to break orbit
       * *****/
      if (s->whatorbits == ScopeLevel::LEVEL_PLAN) {
        dist = sqrt(Distsq(s->xpos, s->ypos, ost.xpos + opl->xpos,
                           ost.ypos + opl->ypos));
        if (dist > PLORBITSIZE) {
          s->whatorbits = ScopeLevel::LEVEL_STAR;
          s->protect.planet = 0;
        }
      } else if (s->whatorbits == ScopeLevel::LEVEL_STAR) {
        dist = sqrt(Distsq(s->xpos, s->ypos, ost.xpos, ost.ypos));
        if (dist > SYSTEMSIZE) {
          s->whatorbits = ScopeLevel::LEVEL_UNIV;
          s->protect.evade = 0;
          s->protect.planet = 0;
        }
      }

      /*******   check for arriving at destination *******/
      if (destlevel == ScopeLevel::LEVEL_STAR ||
          (destlevel == ScopeLevel::LEVEL_PLAN &&
           (s->storbits != deststar ||
            s->whatorbits == ScopeLevel::LEVEL_UNIV))) {
        stardist = sqrt(Distsq(s->xpos, s->ypos, dst.xpos, dst.ypos));
        if (stardist <= SYSTEMSIZE * 1.5) {
          s->whatorbits = ScopeLevel::LEVEL_STAR;
          s->protect.planet = 0;
          s->storbits = deststar;
          /* if this system isn't inhabited by you, give it to the
             governor of the ship */
          if (!checking_fuel && (s->popn || s->type == ShipType::OTYPE_PROBE)) {
            if (!isset(dst.inhabited, s->owner))
              dst.governor[s->owner - 1] = s->governor;
            setbit(dst.explored, s->owner);
            setbit(dst.inhabited, s->owner);
          }
          if (s->type != ShipType::OTYPE_VN) {
            sprintf(telegram_buf, "%s arrived at %s.",
                    ship_to_string(*s).c_str(), prin_ship_orbits(*s).c_str());
            if (send_messages)
              push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
          }
          if (s->whatdest == ScopeLevel::LEVEL_STAR)
            s->whatdest = ScopeLevel::LEVEL_UNIV;
        }
      } else if (destlevel == ScopeLevel::LEVEL_PLAN &&
                 deststar == s->storbits) {
        /* headed for a planet in the same system, & not already there.. */
        dist = sqrt(Distsq(s->xpos, s->ypos, dst.xpos + dpl->xpos,
                           dst.ypos + dpl->ypos));
        if (dist <= PLORBITSIZE) {
          if (!checking_fuel && (s->popn || s->type == ShipType::OTYPE_PROBE)) {
            dpl->info[s->owner - 1].explored = 1;
            setbit(dst.explored, s->owner);
            setbit(dst.inhabited, s->owner);
          }
          s->whatorbits = ScopeLevel::LEVEL_PLAN;
          s->pnumorbits = destpnum;
          if (dist <= (double)DIST_TO_LAND) {
            sprintf(telegram_buf, "%s within landing distance of %s.",
                    ship_to_string(*s).c_str(), prin_ship_orbits(*s).c_str());
            if (checking_fuel || !do_merchant(s, *dpl))
              if (s->whatdest == ScopeLevel::LEVEL_PLAN)
                s->whatdest = ScopeLevel::LEVEL_UNIV;
          } else {
            sprintf(telegram_buf, "%s arriving at %s.",
                    ship_to_string(*s).c_str(), prin_ship_orbits(*s).c_str());
          }
          if (s->type == ShipType::STYPE_OAP) {
            sprintf(buf, "\nEnslavement of the planet is now possible.");
            strcat(telegram_buf, buf);
          }
          if (send_messages && s->type != ShipType::OTYPE_VN)
            push_telegram((int)(s->owner), (int)s->governor, telegram_buf);
        }
      } else if (destlevel == ScopeLevel::LEVEL_SHIP) {
        dist = sqrt(Distsq(s->xpos, s->ypos, dsh->xpos, dsh->ypos));
        if (dist <= PLORBITSIZE) {
          if (dsh->whatorbits == ScopeLevel::LEVEL_PLAN) {
            s->whatorbits = ScopeLevel::LEVEL_PLAN;
            s->storbits = dsh->storbits;
            s->pnumorbits = dsh->pnumorbits;
          } else if (dsh->whatorbits == ScopeLevel::LEVEL_STAR) {
            s->whatorbits = ScopeLevel::LEVEL_STAR;
            s->storbits = dsh->storbits;
            s->protect.planet = 0;
          }
        }
      }
    } /* 'destination' orders */
  } /* if impulse drive */
}

/* deliver an "out of fuel" message.  Used by a number of ship-updating
 *  code segments; so that code isn't duplicated.
 */
void msg_OOF(Ship *s) {
  sprintf(buf, "%s is out of fuel at %s.", ship_to_string(*s).c_str(),
          prin_ship_orbits(*s).c_str());
  push_telegram((int)(s->owner), (int)s->governor, buf);
}

/* followable: returns 1 iff s1 can follow s2 */
int followable(Ship *s1, Ship *s2) {
  double dx;
  double dy;
  double range;
  uint64_t allied;

  if (!s2->alive || !s1->active || s2->whatorbits == ScopeLevel::LEVEL_SHIP)
    return 0;

  dx = s1->xpos - s2->xpos;
  dy = s1->ypos - s2->ypos;

  range = 4.0 * logscale((int)(s1->tech + 1.0)) * SYSTEMSIZE;

  auto &r = races[s2->owner - 1];
  allied = r.allied;
  /* You can follow your own ships, your allies' ships, or nearby ships */
  return (s1->owner == s2->owner) || (isset(allied, s1->owner)) ||
         (sqrt(dx * dx + dy * dy) <= range);
}

/* this routine will do landing, launching, loading, unloading, etc
        for merchant ships. The ship is within landing distance of
        the target Planet */
static int do_merchant(Ship *s, Planet &p) {
  int i;
  int j;
  double fuel;
  char load;
  char unload;
  int amount;

  i = s->owner - 1;
  j = s->merchant - 1; /* try to speed things up a bit */

  if (!s->merchant || !p.info[i].route[j].set) /* not on shipping route */
    return 0;
  /* check to see if the sector is owned by the player */
  auto sect = getsector(p, p.info[i].route[j].x, p.info[i].route[j].y);
  if (sect.owner && (sect.owner != s->owner)) {
    return 0;
  }

  if (!landed(*s)) { /* try to land the ship */
    fuel = s->mass * p.gravity() * LAND_GRAV_MASS_FACTOR;
    if (s->fuel < fuel) { /* ship can't land - cancel all orders */
      s->whatdest = ScopeLevel::LEVEL_UNIV;
      strcat(telegram_buf, "\t\tNot enough fuel to land!\n");
      return 1;
    }
    s->land_x = p.info[i].route[j].x;
    s->land_y = p.info[i].route[j].y;
    sprintf(buf, "\t\tLanded on sector %d,%d\n", s->land_x, s->land_y);
    strcat(telegram_buf, buf);
    s->xpos = p.xpos + stars[s->storbits].xpos;
    s->ypos = p.ypos + stars[s->storbits].ypos;
    use_fuel(*s, fuel);
    s->docked = 1;
    s->whatdest = ScopeLevel::LEVEL_PLAN;
    s->deststar = s->storbits;
    s->destpnum = s->pnumorbits;
  }
  /* load and unload supplies specified by the planet */
  load = p.info[i].route[j].load;
  unload = p.info[i].route[j].unload;
  if (load) {
    strcat(telegram_buf, "\t\t");
    if (Fuel(load)) {
      amount = (int)s->max_fuel - (int)s->fuel;
      if (amount > p.info[i].fuel) amount = p.info[i].fuel;
      p.info[i].fuel -= amount;
      rcv_fuel(*s, (double)amount);
      sprintf(buf, "%df ", amount);
      strcat(telegram_buf, buf);
    }
    if (Resources(load)) {
      amount = (int)s->max_resource - (int)s->resource;
      if (amount > p.info[i].resource) amount = p.info[i].resource;
      p.info[i].resource -= amount;
      rcv_resource(*s, amount);
      sprintf(buf, "%dr ", amount);
      strcat(telegram_buf, buf);
    }
    if (Crystals(load)) {
      amount = p.info[i].crystals;
      p.info[i].crystals -= amount;
      s->crystals += amount;
      sprintf(buf, "%dx ", amount);
      strcat(telegram_buf, buf);
    }
    if (Destruct(load)) {
      amount = (int)s->max_destruct - (int)s->destruct;
      if (amount > p.info[i].destruct) amount = p.info[i].destruct;
      p.info[i].destruct -= amount;
      rcv_destruct(*s, amount);
      sprintf(buf, "%dd ", amount);
      strcat(telegram_buf, buf);
    }
    strcat(telegram_buf, "loaded\n");
  }
  if (unload) {
    strcat(telegram_buf, "\t\t");
    if (Fuel(unload)) {
      amount = (int)s->fuel;
      p.info[i].fuel += amount;
      sprintf(buf, "%df ", amount);
      strcat(telegram_buf, buf);
      use_fuel(*s, (double)amount);
    }
    if (Resources(unload)) {
      amount = s->resource;
      p.info[i].resource += amount;
      sprintf(buf, "%dr ", amount);
      strcat(telegram_buf, buf);
      use_resource(*s, amount);
    }
    if (Crystals(unload)) {
      amount = s->crystals;
      p.info[i].crystals += amount;
      sprintf(buf, "%dx ", amount);
      strcat(telegram_buf, buf);
      s->crystals -= amount;
    }
    if (Destruct(unload)) {
      amount = s->destruct;
      p.info[i].destruct += amount;
      sprintf(buf, "%dd ", amount);
      strcat(telegram_buf, buf);
      use_destruct(*s, amount);
    }
    strcat(telegram_buf, "unloaded\n");
  }

  /* launch the ship */
  fuel = s->mass * p.gravity() * LAUNCH_GRAV_MASS_FACTOR;
  if (s->fuel < fuel) {
    strcat(telegram_buf, "\t\tNot enough fuel to launch!\n");
    return 1;
  }
  /* ship is ready to fly - order the ship to its next destination */
  s->whatdest = ScopeLevel::LEVEL_PLAN;
  s->deststar = p.info[i].route[j].dest_star;
  s->destpnum = p.info[i].route[j].dest_planet;
  s->docked = 0;
  use_fuel(*s, fuel);
  sprintf(buf, "\t\tDestination set to %s\n", prin_ship_dest(*s).c_str());
  strcat(telegram_buf, buf);
  if (s->hyper_drive.has) { /* order the ship to jump if it can */
    if (s->storbits != s->deststar) {
      s->navigate.on = 0;
      s->hyper_drive.on = 1;
      if (s->mounted) {
        s->hyper_drive.charge = 1;
        s->hyper_drive.ready = 1;
      } else {
        s->hyper_drive.charge = 0;
        s->hyper_drive.ready = 0;
      }
      strcat(telegram_buf, "\t\tJump orders set\n");
    }
  }
  return 1;
}
