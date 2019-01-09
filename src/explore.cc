// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "explore.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "getplace.h"
#include "map.h"
#include "max.h"
#include "power.h"
#include "races.h"
#include "ships.h"
#include "tech.h"
#include "tweakables.h"
#include "vars.h"

enum modes_t { COLONIES, PRODUCTION };

static void tech_report_star(int, int, startype *, starnum_t, int *, double *,
                             double *);

static void colonies_at_star(GameObj &g, racetype *Race, starnum_t star,
                             modes_t mode) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  planetnum_t i;
  int j;

  getstar(&(Stars[star]), star);
  if (!isset(Stars[star]->explored, Playernum)) return;

  for (i = 0; i < Stars[star]->numplanets; i++) {
    const auto pl = getplanet(star, i);

    if (pl.info[Playernum - 1].explored &&
        pl.info[Playernum - 1].numsectsowned &&
        (!Governor || Stars[star]->governor[Playernum - 1] == Governor)) {
      switch (mode) {
        case COLONIES:
          sprintf(
              buf,
              " %c %4.4s/%-4.4s%c%4d%3d%5d%8ld%3d%6lu%5d%6d "
              "%3d/%-3d%3.0f/%-3d%3d/%-3d",
              Psymbol[pl.type], Stars[star]->name, Stars[star]->pnames[i],
              (pl.info[Playernum - 1].autorep ? '*' : ' '),
              Stars[star]->governor[Playernum - 1],
              pl.info[Playernum - 1].numsectsowned,
              pl.info[Playernum - 1].tech_invest, pl.info[Playernum - 1].popn,
              pl.info[Playernum - 1].crystals, pl.info[Playernum - 1].resource,
              pl.info[Playernum - 1].destruct, pl.info[Playernum - 1].fuel,
              pl.info[Playernum - 1].tax, pl.info[Playernum - 1].newtax,
              compatibility(pl, Race), pl.conditions[TOXIC],
              pl.info[Playernum - 1].comread, pl.info[Playernum - 1].mob_set);
          notify(Playernum, Governor, buf);
          for (j = 1; j <= Num_races; j++)
            if ((j != Playernum) && (pl.info[j - 1].numsectsowned > 0)) {
              sprintf(buf, " %d", j);
              notify(Playernum, Governor, buf);
            }
          g.out << "\n";
          if (mode == 0) break;
          [[clang::fallthrough]]; /* Fall through if (mode == -1) */
        case PRODUCTION:
          sprintf(
              buf,
              " %c %4.4s/%-4.4s%c%3d%8.4f%8ld%3d%6d%5d%6d %6ld   %3d%8.2f\n",
              Psymbol[pl.type], Stars[star]->name, Stars[star]->pnames[i],
              (pl.info[Playernum - 1].autorep ? '*' : ' '),
              Stars[star]->governor[Playernum - 1],
              pl.info[Playernum - 1].prod_tech, pl.total_resources,
              pl.info[Playernum - 1].prod_crystals,
              pl.info[Playernum - 1].prod_res, pl.info[Playernum - 1].prod_dest,
              pl.info[Playernum - 1].prod_fuel,
              pl.info[Playernum - 1].prod_money,
              pl.info[Playernum - 1].tox_thresh,
              pl.info[Playernum - 1].est_production);
          notify(Playernum, Governor, buf);
          break;
      }
    }
  }
}

void colonies(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int i;
  racetype *Race;
  placetype where;

  modes_t mode;

  if (argv[0] == "colonies")
    mode = COLONIES;
  else
    mode = PRODUCTION;

  switch (mode) {
    case COLONIES:
      notify(Playernum, Governor,
             "          ========== Colonization Report ==========\n");
      notify(Playernum, Governor,
             "  Planet     gov sec tech    popn  x   res  "
             "des  fuel  tax  cmpt/tox mob  Aliens\n");
      break;
    case PRODUCTION:
      notify(Playernum, Governor,
             "          ============ Production Report ==========\n");
      notify(Playernum, Governor,
             "  Planet     gov    tech deposit  x   res  "
             "des  fuel    tax   tox  est prod\n");
      break;
  }

  Race = races[Playernum - 1];
  getsdata(&Sdata);

  if (argv.size() < 2)
    for (starnum_t star = 0; star < Sdata.numstars; star++)
      colonies_at_star(g, Race, star, mode);
  else
    for (i = 1; i < argv.size(); i++) {
      where = Getplace(g, argv[i], 0);
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "Bad location `%s'.\n", argv[i].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      colonies_at_star(g, Race, where.snum, mode);
    }
  g.out << "\n";
}

void distance(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  placetype from;
  placetype to;
  double x0;
  double y0;
  double x1;
  double y1;
  double dist;

  if (argv.size() < 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return;
  }

  from = Getplace(g, argv[1], 1);
  if (from.err) {
    sprintf(buf, "Bad scope '%s'.\n", argv[1].c_str());
    notify(Playernum, Governor, buf);
    return;
  }
  to = Getplace(g, argv[2], 1);
  if (to.err) {
    sprintf(buf, "Bad scope '%s'.\n", argv[2].c_str());
    notify(Playernum, Governor, buf);
  }

  x0 = 0.0;
  y0 = 0.0;
  x1 = 0.0;
  y1 = 0.0;
  /* get position in absolute units */
  if (from.level == ScopeLevel::LEVEL_SHIP) {
    auto ship = getship(from.shipno);
    if (ship->owner != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x0 = ship->xpos;
    y0 = ship->ypos;
  } else if (from.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(from.snum, from.pnum);
    x0 = p.xpos + Stars[from.snum]->xpos;
    y0 = p.ypos + Stars[from.snum]->ypos;
  } else if (from.level == ScopeLevel::LEVEL_STAR) {
    x0 = Stars[from.snum]->xpos;
    y0 = Stars[from.snum]->ypos;
  }

  if (to.level == ScopeLevel::LEVEL_SHIP) {
    auto ship = getship(to.shipno);
    if (ship->owner != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x1 = ship->xpos;
    y1 = ship->ypos;
  } else if (to.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(to.snum, to.pnum);
    x1 = p.xpos + Stars[to.snum]->xpos;
    y1 = p.ypos + Stars[to.snum]->ypos;
  } else if (to.level == ScopeLevel::LEVEL_STAR) {
    x1 = Stars[to.snum]->xpos;
    y1 = Stars[to.snum]->ypos;
  }
  /* compute the distance */
  dist = sqrt(Distsq(x0, y0, x1, y1));
  sprintf(buf, "Distance = %f\n", dist);
  notify(Playernum, Governor, buf);
}

void star_locations(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int i;
  double dist;
  double x;
  double y;
  int max;

  x = g.lastx[1];
  y = g.lasty[1];

  if (argv.size() > 1)
    max = std::stoi(argv[1]);
  else
    max = 999999;

  for (i = 0; i < Sdata.numstars; i++) {
    dist = sqrt(Distsq(Stars[i]->xpos, Stars[i]->ypos, x, y));
    if ((int)dist <= max) {
      sprintf(buf, "(%2d) %20.20s (%8.0f,%8.0f) %7.0f\n", i, Stars[i]->name,
              Stars[i]->xpos, Stars[i]->ypos, dist);
      notify(Playernum, Governor, buf);
    }
  }
}

void exploration(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int starq;
  int j;
  placetype where;
  racetype *Race;

  starq = -1;

  if (argv.size() == 2) {
    where = Getplace(g, argv[1], 0);
    if (where.err) {
      sprintf(buf, "explore: bad scope.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    if (where.level == ScopeLevel::LEVEL_SHIP ||
        where.level == ScopeLevel::LEVEL_UNIV) {
      sprintf(buf, "Bad scope '%s'.\n", argv[1].c_str());
      notify(Playernum, Governor, buf);
      return;
    }
    starq = where.snum;
  }

  Race = races[Playernum - 1];

  getsdata(&Sdata);
  sprintf(buf, "         ========== Exploration Report ==========\n");
  notify(Playernum, Governor, buf);
  sprintf(buf, " Global action points : [%2d]\n", Sdata.AP[Playernum - 1]);
  notify(Playernum, Governor, buf);
  sprintf(
      buf,
      " Star  (stability)[AP]   #  Planet [Attributes] Type (Compatibility)\n");
  notify(Playernum, Governor, buf);
  for (starnum_t star = 0; star < Sdata.numstars; star++)
    if ((starq == -1) || (starq == star)) {
      getstar(&(Stars[star]), star);
      if (isset(Stars[star]->explored, Playernum))
        for (planetnum_t i = 0; i < Stars[star]->numplanets; i++) {
          const auto pl = getplanet(star, i);
          if (i == 0) {
            if (Race->tech >= TECH_SEE_STABILITY) {
              sprintf(buf, "\n%13s (%2d)[%2d]\n", Stars[star]->name,
                      Stars[star]->stability, Stars[star]->AP[Playernum - 1]);
              notify(Playernum, Governor, buf);
            } else {
              sprintf(buf, "\n%13s (/?/?)[%2d]\n", Stars[star]->name,
                      Stars[star]->AP[Playernum - 1]);
              notify(Playernum, Governor, buf);
            }
          }

          sprintf(buf, "\t\t      ");
          notify(Playernum, Governor, buf);

          sprintf(buf, "  #%d. %-15s [ ", i + 1, Stars[star]->pnames[i]);
          notify(Playernum, Governor, buf);
          if (pl.info[Playernum - 1].explored) {
            sprintf(buf, "Ex ");
            notify(Playernum, Governor, buf);
            if (pl.info[Playernum - 1].autorep) {
              sprintf(buf, "Rep ");
              notify(Playernum, Governor, buf);
            }
            if (pl.info[Playernum - 1].numsectsowned) {
              sprintf(buf, "Inhab ");
              notify(Playernum, Governor, buf);
            }
            if (pl.slaved_to) {
              sprintf(buf, "SLAVED ");
              notify(Playernum, Governor, buf);
            }
            for (j = 1; j <= Num_races; j++)
              if (j != Playernum && pl.info[j - 1].numsectsowned) {
                sprintf(buf, "%d ", j);
                notify(Playernum, Governor, buf);
              }
            if (pl.conditions[TOXIC] > 70) {
              sprintf(buf, "TOXIC ");
              notify(Playernum, Governor, buf);
            }
            sprintf(buf, "] %s %2.0f%%\n", Planet_types[pl.type],
                    compatibility(pl, Race));
            notify(Playernum, Governor, buf);
          } else {
            sprintf(buf, "No Data ]\n");
            notify(Playernum, Governor, buf);
          }
        }
    }
}

void tech_status(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int k;
  placetype where;
  double total_gain = 0.0;
  double total_max_gain = 0.0;
  int total_invest = 0;

  getsdata(&Sdata);

  sprintf(buf, "             ========== Technology Report ==========\n\n");
  notify(Playernum, Governor, buf);

  sprintf(buf, "       Planet          popn    invest    gain   ^gain\n");
  notify(Playernum, Governor, buf);

  if (argv.size() == 1) {
    for (starnum_t star = 0; star < Sdata.numstars; star++) {
      getstar(&(Stars[star]), star);
      tech_report_star(Playernum, Governor, Stars[star], star, &total_invest,
                       &total_gain, &total_max_gain);
    }
  } else { /* Several arguments */
    for (k = 1; k < argv.size(); k++) {
      where = Getplace(g, argv[k], 0);
      if (where.err || where.level == ScopeLevel::LEVEL_UNIV ||
          where.level == ScopeLevel::LEVEL_SHIP) {
        sprintf(buf, "Bad location `%s'.\n", argv[k].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      starnum_t star = where.snum;
      getstar(&Stars[star], star);
      tech_report_star(Playernum, Governor, Stars[star], star, &total_invest,
                       &total_gain, &total_max_gain);
    }
  }
  sprintf(buf, "       Total Popn:  %7ld\n", Power[Playernum - 1].popn);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Tech: %31d%8.3lf%8.3lf\n", total_invest, total_gain,
          total_max_gain);
  notify(Playernum, Governor, buf);
}

static void tech_report_star(int Playernum, int Governor, startype *star,
                             starnum_t snum, int *t_invest, double *t_gain,
                             double *t_max_gain) {
  char str[200];
  double gain;
  double max_gain;

  if (isset(star->explored, Playernum) &&
      (!Governor || star->governor[Playernum - 1] == Governor)) {
    for (planetnum_t i = 0; i < star->numplanets; i++) {
      const auto pl = getplanet(snum, i);
      if (pl.info[Playernum - 1].explored &&
          pl.info[Playernum - 1].numsectsowned) {
        sprintf(str, "%s/%s%s", star->name, star->pnames[i],
                (pl.info[Playernum - 1].autorep ? "*" : ""));
        sprintf(buf, "%16.16s %10ld%10d%8.3lf%8.3lf\n", str,
                pl.info[Playernum - 1].popn, pl.info[Playernum - 1].tech_invest,
                gain = tech_prod((int)pl.info[Playernum - 1].tech_invest,
                                 (int)pl.info[Playernum - 1].popn),
                max_gain = tech_prod((int)pl.info[Playernum - 1].prod_res,
                                     (int)pl.info[Playernum - 1].popn));
        notify(Playernum, Governor, buf);
        *t_invest += pl.info[Playernum - 1].tech_invest;
        *t_gain += gain;
        *t_max_gain += max_gain;
      }
    }
  }
}
