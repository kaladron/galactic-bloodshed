// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  orbit.c -- display orbits of planets (graphic representation) */

import gblib;
import std;

#include "gb/commands/orbit.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static double Lastx, Lasty, Zoom;
static const int SCALE = 100;

static void DispStar(const GameObj &, const ScopeLevel, const Star &, int,
                     const Race &, char *);
static void DispPlanet(const GameObj &, const ScopeLevel, const Planet &,
                       char *, int, const Race &, char *);
static void DispShip(const GameObj &, const Place &, Ship *, const Race &,
                     char *, const Planet & = Planet());

/* OPTIONS
 *  -p : If this option is set, ``orbit'' will not display planet names.
 *
 *  -S : Do not display star names.
 *
 *  -s : Do not display ships.
 *
 *  -(number) : Do not display that #'d ship or planet (in case it obstructs
 * 		the view of another object)
 */
void orbit(const command_t &argv, GameObj &g) {
  int DontDispNum = -1;
  int DontDispPlanets;
  int DontDispShips;
  int DontDispStars;
  char output[100000];

  DontDispPlanets = DontDispShips = DontDispStars = 0;

  /* find options, set flags accordingly */
  for (int flag = 1; flag <= argv.size() - 1; flag++)
    if (*argv[flag].c_str() == '-') {
      for (int i = 1; argv[flag][i] != '\0'; i++) switch (argv[flag][i]) {
          case 's':
            DontDispShips = 1;
            break;
          case 'S':
            DontDispStars = 1;
            break;
          case 'p':
            DontDispPlanets = 1;
            break;
          default:
            if (sscanf(argv[flag].c_str() + 1, "%d", &DontDispNum) != 1) {
              sprintf(buf, "Bad number %s.\n", argv[flag].c_str() + 1);
              notify(g.player, g.governor, buf);
              DontDispNum = -1;
            }
            if (DontDispNum) DontDispNum--; /* make a '1' into a '0' */
            break;
        }
    }

  std::unique_ptr<Place> where;
  if (argv.size() == 1) {
    where = std::make_unique<Place>(g, ":");
    int i = (g.level == ScopeLevel::LEVEL_UNIV);
    Lastx = g.lastx[i];
    Lasty = g.lasty[i];
    Zoom = g.zoom[i];
  } else {
    where = std::make_unique<Place>(g, argv[argv.size() - 1]);
    Lastx = Lasty = 0.0;
    Zoom = 1.1;
  }

  if (where->err) {
    notify(g.player, g.governor, "orbit: error in args.\n");
    return;
  }

  /* orbit type of map */
  sprintf(output, "#");

  auto Race = races[g.player - 1];

  switch (where->level) {
    case ScopeLevel::LEVEL_UNIV:
      for (starnum_t i = 0; i < Sdata.numstars; i++)
        if (DontDispNum != i) {
          DispStar(g, ScopeLevel::LEVEL_UNIV, stars[i], DontDispStars, Race,
                   buf);
          strcat(output, buf);
        }
      if (!DontDispShips) {
        Shiplist shiplist{Sdata.ships};
        for (auto &s : shiplist) {
          if (DontDispNum != s.number) {
            DispShip(g, *where, &s, Race, buf);
            strcat(output, buf);
          }
        }
      }
      break;
    case ScopeLevel::LEVEL_STAR: {
      DispStar(g, ScopeLevel::LEVEL_STAR, stars[where->snum], DontDispStars,
               Race, buf);
      strcat(output, buf);

      for (planetnum_t i = 0; i < stars[where->snum].numplanets; i++)
        if (DontDispNum != i) {
          const auto p = getplanet(where->snum, i);
          DispPlanet(g, ScopeLevel::LEVEL_STAR, p, stars[where->snum].pnames[i],
                     DontDispPlanets, Race, buf);
          strcat(output, buf);
        }
      /* check to see if you have ships at orbiting the star, if so you can
         see enemy ships */
      bool iq = false;
      if (g.god)
        iq = true;
      else {
        Shiplist shiplist{stars[where->snum].ships};
        for (auto &s : shiplist) {
          if (s.owner == g.player && shipsight(s)) {
            iq = true; /* you are there to sight, need a crew */
            break;
          }
        }
      }
      if (!DontDispShips) {
        Shiplist shiplist{stars[where->snum].ships};
        for (auto &s : shiplist) {
          if (DontDispNum != s.number &&
              !(s.owner != g.player && s.type == ShipType::STYPE_MINE)) {
            if ((s.owner == g.player) || iq) {
              DispShip(g, *where, &s, Race, buf);
              strcat(output, buf);
            }
          }
        }
      }
    } break;
    case ScopeLevel::LEVEL_PLAN: {
      const auto p = getplanet(where->snum, where->pnum);
      DispPlanet(g, ScopeLevel::LEVEL_PLAN, p,
                 stars[where->snum].pnames[where->pnum], DontDispPlanets, Race,
                 buf);
      strcat(output, buf);

      /* check to see if you have ships at landed or
         orbiting the planet, if so you can see orbiting enemy ships */
      bool iq = false;
      Shiplist shiplist{p.ships};
      for (auto &s : shiplist) {
        if (s.owner == g.player && shipsight(s)) {
          iq = true; /* you are there to sight, need a crew */
          break;
        }
      }
      /* end check */
      if (!DontDispShips) {
        for (auto &s : shiplist) {
          if (DontDispNum != s.number) {
            if (!landed(s)) {
              if ((s.owner == g.player) || iq) {
                DispShip(g, *where, &s, Race, buf, p);
                strcat(output, buf);
              }
            }
          }
        }
      }
    } break;
    default:
      g.out << "Bad scope.\n";
      return;
  }
  strcat(output, "\n");
  notify(g.player, g.governor, output);
}

// TODO(jeffbailey) Remove DontDispStar parameter as unused, but it really looks
// like we should be doing something here.
static void DispStar(const GameObj &g, const ScopeLevel level, const Star &star,
                     int /* DontDispStars */, const Race &r, char *string) {
  int x = 0;  // TODO(jeffbailey): Inititalized x and y to 0.
  int y = 0;
  int stand;

  *string = '\0';

  if (level == ScopeLevel::LEVEL_UNIV) {
    x = (int)(SCALE + ((SCALE * (star.xpos - Lastx)) / (UNIVSIZE * Zoom)));
    y = (int)(SCALE + ((SCALE * (star.ypos - Lasty)) / (UNIVSIZE * Zoom)));
  } else if (level == ScopeLevel::LEVEL_STAR) {
    x = (int)(SCALE + (SCALE * (-Lastx)) / (SYSTEMSIZE * Zoom));
    y = (int)(SCALE + (SCALE * (-Lasty)) / (SYSTEMSIZE * Zoom));
  }
  /*if (star->nova_stage)
    DispArray(x, y, 11,7, Novae[star->nova_stage-1], fac); */
  if (y >= 0 && x >= 0) {
    if (r.governor[g.governor].toggle.color) {
      stand = (isset(star.explored, g.player) ? g.player : 0) + '?';
      sprintf(temp, "%c %d %d 0 * ", (char)stand, x, y);
      strcat(string, temp);
      stand = (isset(star.inhabited, g.player) ? g.player : 0) + '?';
      sprintf(temp, "%c %s;", (char)stand, star.name);
      strcat(string, temp);
    } else {
      stand = (isset(star.explored, g.player) ? 1 : 0);
      sprintf(temp, "%d %d %d 0 * ", stand, x, y);
      strcat(string, temp);
      stand = (isset(star.inhabited, g.player) ? 1 : 0);
      sprintf(temp, "%d %s;", stand, star.name);
      strcat(string, temp);
    }
  }
}

// TODO(jeffbailey): We remove DontDispPlanets as unused, but it really seems
// like we should be doing something here!
static void DispPlanet(const GameObj &g, const ScopeLevel level,
                       const Planet &p, char *name, int /* DontDispPlanets */,
                       const Race &r, char *string) {
  int x = 0;  // TODO(jeffbailey): Check if init to 0 is right.
  int y = 0;
  int stand;

  *string = '\0';

  if (level == ScopeLevel::LEVEL_STAR) {
    y = (int)(SCALE + (SCALE * (p.ypos - Lasty)) / (SYSTEMSIZE * Zoom));
    x = (int)(SCALE + (SCALE * (p.xpos - Lastx)) / (SYSTEMSIZE * Zoom));
  } else if (level == ScopeLevel::LEVEL_PLAN) {
    y = (int)(SCALE + (SCALE * (-Lasty)) / (PLORBITSIZE * Zoom));
    x = (int)(SCALE + (SCALE * (-Lastx)) / (PLORBITSIZE * Zoom));
  }
  if (x >= 0 && y >= 0) {
    if (r.governor[g.governor].toggle.color) {
      stand = (p.info[g.player - 1].explored ? g.player : 0) + '?';
      sprintf(temp, "%c %d %d 0 %c ", (char)stand, x, y,
              (stand > '0' ? Psymbol[p.type] : '?'));
      strcat(string, temp);
      stand = (p.info[g.player - 1].numsectsowned ? g.player : 0) + '?';
      sprintf(temp, "%c %s", (char)stand, name);
      strcat(string, temp);
    } else {
      stand = p.info[g.player - 1].explored ? 1 : 0;
      sprintf(temp, "%d %d %d 0 %c ", stand, x, y,
              (stand ? Psymbol[p.type] : '?'));
      strcat(string, temp);
      stand = p.info[g.player - 1].numsectsowned ? 1 : 0;
      sprintf(temp, "%d %s", stand, name);
      strcat(string, temp);
    }
    if (r.governor[g.governor].toggle.compat && p.info[g.player - 1].explored) {
      sprintf(temp, "(%d)", (int)p.compatibility(r));
      strcat(string, temp);
    }
    strcat(string, ";");
  }
}

static void DispShip(const GameObj &g, const Place &where, Ship *ship,
                     const Race &r, char *string, const Planet &pl) {
  int x;
  int y;
  int wm;
  int stand;
  double xt;
  double yt;
  double slope;

  if (!ship->alive) return;

  *string = '\0';

  switch (where.level) {
    case ScopeLevel::LEVEL_PLAN:
      x = (int)(SCALE + (SCALE * (ship->xpos -
                                  (stars[where.snum].xpos + pl.xpos) - Lastx)) /
                            (PLORBITSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship->ypos -
                                  (stars[where.snum].ypos + pl.ypos) - Lasty)) /
                            (PLORBITSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_STAR:
      x = (int)(SCALE +
                (SCALE * (ship->xpos - stars[where.snum].xpos - Lastx)) /
                    (SYSTEMSIZE * Zoom));
      y = (int)(SCALE +
                (SCALE * (ship->ypos - stars[where.snum].ypos - Lasty)) /
                    (SYSTEMSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_UNIV:
      x = (int)(SCALE + (SCALE * (ship->xpos - Lastx)) / (UNIVSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship->ypos - Lasty)) / (UNIVSIZE * Zoom));
      break;
    default:
      notify(g.player, g.governor, "WHOA! error in DispShip.\n");
      return;
  }

  switch (ship->type) {
    case ShipType::STYPE_MIRROR:
      if (ship->special.aimed_at.level == ScopeLevel::LEVEL_STAR) {
        xt = stars[ship->special.aimed_at.snum].xpos;
        yt = stars[ship->special.aimed_at.snum].ypos;
      } else if (ship->special.aimed_at.level == ScopeLevel::LEVEL_PLAN) {
        if (where.level == ScopeLevel::LEVEL_PLAN &&
            ship->special.aimed_at.pnum == where.pnum) {
          /* same planet */
          xt = stars[ship->special.aimed_at.snum].xpos + pl.xpos;
          yt = stars[ship->special.aimed_at.snum].ypos + pl.ypos;
        } else { /* different planet */
          const auto apl = getplanet(where.snum, where.pnum);
          xt = stars[ship->special.aimed_at.snum].xpos + apl.xpos;
          yt = stars[ship->special.aimed_at.snum].ypos + apl.ypos;
        }
      } else if (ship->special.aimed_at.level == ScopeLevel::LEVEL_SHIP) {
        auto aship = getship(ship->special.aimed_at.shipno);
        if (aship) {
          xt = aship->xpos;
          yt = aship->ypos;
        } else
          xt = yt = 0.0;
      } else
        xt = yt = 0.0;
      wm = 0;

      if (xt == ship->xpos) {
        if (yt > ship->ypos)
          wm = 4;
        else
          wm = 0;
      } else {
        slope = (yt - ship->ypos) / (xt - ship->xpos);
        if (yt == ship->ypos) {
          if (xt > ship->xpos)
            wm = 2;
          else
            wm = 6;
        } else if (yt > ship->ypos) {
          if (slope < -2.414) wm = 4;
          if (slope > -2.414) wm = 5;
          if (slope > -0.414) wm = 6;
          if (slope > 0.000) wm = 2;
          if (slope > 0.414) wm = 3;
          if (slope > 2.414) wm = 4;
        } else if (yt < ship->ypos) {
          if (slope < -2.414) wm = 0;
          if (slope > -2.414) wm = 1;
          if (slope > -0.414) wm = 2;
          if (slope > 0.000) wm = 6;
          if (slope > 0.414) wm = 7;
          if (slope > 2.414) wm = 0;
        }
      }

      /* (magnification) */
      if (x >= 0 && y >= 0) {
        if (r.governor[g.governor].toggle.color) {
          sprintf(string, "%c %d %d %d %c %c %lu;", (char)(ship->owner + '?'),
                  x, y, wm, Shipltrs[ship->type], (char)(ship->owner + '?'),
                  ship->number);
        } else {
          stand = (ship->owner == r.governor[g.governor].toggle.highlight);
          sprintf(string, "%d %d %d %d %c %d %lu;", stand, x, y, wm,
                  Shipltrs[ship->type], stand, ship->number);
        }
      }
      break;

    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      break;

    default:
      /* other ships can only be seen when in system */
      wm = 0;
      if (ship->whatorbits != ScopeLevel::LEVEL_UNIV ||
          ((ship->owner == g.player) || g.god))
        if (x >= 0 && y >= 0) {
          if (r.governor[g.governor].toggle.color) {
            sprintf(string, "%c %d %d %d %c %c %lu;", (char)(ship->owner + '?'),
                    x, y, wm, Shipltrs[ship->type], (char)(ship->owner + '?'),
                    ship->number);
          } else {
            stand = (ship->owner == r.governor[g.governor].toggle.highlight);
            sprintf(string, "%d %d %d %d %c %d %lu;", stand, x, y, wm,
                    Shipltrs[ship->type], stand, ship->number);
          }
        }
      break;
  }
}
