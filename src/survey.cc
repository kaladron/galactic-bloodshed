// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* survey.c -- print out survey for planets */

#include "survey.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "GB_server.h"
#include "buffers.h"
#include "csp.h"
#include "csp_types.h"
#include "files_shl.h"
#include "fire.h"
#include "get4args.h"
#include "getplace.h"
#include "map.h"
#include "max.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

#define MAX_SHIPS_PER_SECTOR 10

static const char *Tox[] = {
    "Stage 0, mild",
    "Stage 1, mild",
    "Stage 2, semi-mild",
    "Stage 3, semi-semi mild",
    "Stage 4, ecologically unsound",
    "Stage 5: ecologically unsound",
    "Stage 6: below birth threshold",
    "Stage 7: ecologically unstable--below birth threshold",
    "Stage 8: ecologically poisonous --below birth threshold",
    "Stage 9: WARNING: nearing 100% toxicity",
    "Stage 10: WARNING: COMPLETELY TOXIC!!!",
    "???"};

void survey(const command_t &argv, const player_t Playernum,
            const governor_t Governor) {
  int lowx, hix, lowy, hiy, x2;
  char d;
  char sect_char;
  sectortype *s;
  planettype *p;
  int tindex;
  placetype where;
  double compat;
  int avg_fert, avg_resource;
  int crystal_count;
  racetype *Race;
  int all = 0; /* full survey 1, specific 0 */
  struct numshipstuff {
    int pos;
    struct shipstuff {
      int shipno;
      char ltr;
      unsigned char owner;
    } shipstuffs[MAX_SHIPS_PER_SECTOR];
  };
  struct numshipstuff shiplocs[MAX_X][MAX_Y];
  int inhere = 0;  // TODO(jeffbailey): Force init for some cases below
  int shiplist;
  shiptype *shipa;
  int i;

  int mode;
  if (argv[0] == "survey")
    mode = 0;
  else
    mode = 1;

  /* general code -- jpd -- */

  if (argn == 1) { /* no args */
    where.level = Dir[Playernum - 1][Governor].level;
    where.snum = Dir[Playernum - 1][Governor].snum;
    where.pnum = Dir[Playernum - 1][Governor].pnum;
  } else {
    /* they are surveying a sector */
    if ((isdigit(args[1][0]) && index(args[1], ',') != NULL) ||
        ((*args[1] == '-') && (all = 1))) {
      if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
        sprintf(buf, "There are no sectors here.\n");
        notify(Playernum, Governor, buf);
        return;
      } else {
        where.level = LEVEL_PLAN;
        where.snum = Dir[Playernum - 1][Governor].snum;
        where.pnum = Dir[Playernum - 1][Governor].pnum;
      }
    } else {
      where = Getplace(Playernum, Governor, args[1], 0);
      if (where.err || where.level == LEVEL_SHIP) return;
    }
  }

  Race = races[Playernum - 1];

  if (where.level == LEVEL_PLAN) {
    getplanet(&p, (int)where.snum, (int)where.pnum);

    compat = compatibility(p, Race);

    if ((isdigit(args[1][0]) && index(args[1], ',') != NULL) || all) {
      getsmap(Smap, p);

      if (!all) {
        get4args(args[1], &x2, &hix, &lowy, &hiy);
        /* ^^^ translate from lowx:hix,lowy:hiy */
        x2 = std::max(0, x2);
        hix = std::min(hix, p->Maxx - 1);
        lowy = std::max(0, lowy);
        hiy = std::min(hiy, p->Maxy - 1);
      } else {
        x2 = 0;
        hix = p->Maxx - 1;
        lowy = 0;
        hiy = p->Maxy - 1;
      }

      if (!mode) {
        sprintf(buf,
                " x,y cond/type  owner race eff mob frt  res  mil popn "
                "^popn xtals\n");
        notify(Playernum, Governor, buf);
      }
      if (mode) {
        if (all) {
          sprintf(buf, "%c %d %d %d %s %s %d %d %d %ld %ld %d %.2f %d\n",
                  CSP_CLIENT, CSP_SURVEY_INTRO, p->Maxx, p->Maxy,
                  Stars[where.snum]->name,
                  Stars[where.snum]->pnames[where.pnum],
                  p->info[Playernum - 1].resource, p->info[Playernum - 1].fuel,
                  p->info[Playernum - 1].destruct, p->popn, p->maxpopn,
                  p->conditions[TOXIC], compatibility(p, Race), p->slaved_to);
          notify(Playernum, Governor, buf);
        }
        bzero((struct shipstuff *)shiplocs, sizeof(shiplocs));
        inhere = p->info[Playernum - 1].numsectsowned;
        shiplist = p->ships;
        while (shiplist) {
          (void)getship(&shipa, shiplist);
          if (shipa->owner == Playernum &&
              (shipa->popn || (shipa->type == OTYPE_PROBE)))
            inhere = 1;
          if (shipa->alive && landed(shipa) &&
              shiplocs[shipa->land_x][shipa->land_y].pos <
                  MAX_SHIPS_PER_SECTOR) {
            shiplocs[shipa->land_x][shipa->land_y]
                .shipstuffs[shiplocs[shipa->land_x][shipa->land_y].pos]
                .shipno = shiplist;
            shiplocs[shipa->land_x][shipa->land_y]
                .shipstuffs[shiplocs[shipa->land_x][shipa->land_y].pos]
                .owner = shipa->owner;
            shiplocs[shipa->land_x][shipa->land_y]
                .shipstuffs[shiplocs[shipa->land_x][shipa->land_y].pos]
                .ltr = Shipltrs[shipa->type];
            shiplocs[shipa->land_x][shipa->land_y].pos++;
          }
          shiplist = shipa->nextship;
          free((char *)shipa);
        }
      }
      for (; lowy <= hiy; lowy++)
        for (lowx = x2; lowx <= hix; lowx++) {
          s = &Sector(*p, lowx, lowy);
          /* if (s->owner==Playernum) */
          if (!mode) {
            sprintf(buf, "%2d,%-2d ", lowx, lowy);
            notify(Playernum, Governor, buf);
            if ((d = desshow(Playernum, Governor, p, lowx, lowy, Race)) ==
                CHAR_CLOAKED) {
              sprintf(buf, "?  (    ?    )\n");
              notify(Playernum, Governor, buf);
            } else {
              sprintf(
                  buf, " %c   %c   %6u%5u%4u%4u%4u%5u%5lu%5lu%6d%s\n",
                  Dessymbols[s->condition], Dessymbols[s->type], s->owner,
                  s->race, s->eff, s->mobilization, s->fert, s->resource,
                  s->troops, s->popn,
                  maxsupport(Race, s, compat, p->conditions[TOXIC]),
                  ((s->crystals && (Race->discoveries[D_CRYSTAL] || Race->God))
                       ? " yes"
                       : " "));
              notify(Playernum, Governor, buf);
            }
          } else { /* mode */
            switch (s->condition) {
              case SEA:
                sect_char = CHAR_SEA;
                break;
              case LAND:
                sect_char = CHAR_LAND;
                break;
              case MOUNT:
                sect_char = CHAR_MOUNT;
                break;
              case GAS:
                sect_char = CHAR_GAS;
                break;
              case PLATED:
                sect_char = CHAR_PLATED;
                break;
              case ICE:
                sect_char = CHAR_ICE;
                break;
              case DESERT:
                sect_char = CHAR_DESERT;
                break;
              case FOREST:
                sect_char = CHAR_FOREST;
                break;
              default:
                sect_char = '?';
                break;
            }
            sprintf(
                buf, "%c %d %d %d %c %c %d %u %u %u %u %d %u %lu %lu %d",
                CSP_CLIENT, CSP_SURVEY_SECTOR, lowx, lowy, sect_char,
                desshow(Playernum, Governor, p, lowx, lowy, Race),
                ((s->condition == WASTED) ? 1 : 0), s->owner, s->eff, s->fert,
                s->mobilization,
                ((s->crystals && (Race->discoveries[D_CRYSTAL] || Race->God))
                     ? 1
                     : 0),
                s->resource, s->popn, s->troops,
                maxsupport(Race, s, compat, p->conditions[TOXIC]));
            notify(Playernum, Governor, buf);

            if (shiplocs[lowx][lowy].pos && inhere) {
              notify(Playernum, Governor, ";");
              for (i = 0; i < shiplocs[lowx][lowy].pos; i++) {
                sprintf(buf, " %d %c %u;",
                        shiplocs[lowx][lowy].shipstuffs[i].shipno,
                        shiplocs[lowx][lowy].shipstuffs[i].ltr,
                        shiplocs[lowx][lowy].shipstuffs[i].owner);
                notify(Playernum, Governor, buf);
              }
            }
            notify(Playernum, Governor, "\n");
          }
        }
      if (mode) {
        sprintf(buf, "%c %d\n", CSP_CLIENT, CSP_SURVEY_END);
        notify(Playernum, Governor, buf);
      }
    } else {
      /* survey of planet */
      sprintf(buf, "%s:\n", Stars[where.snum]->pnames[where.pnum]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "gravity   x,y absolute     x,y relative to %s\n",
              Stars[where.snum]->name);
      notify(Playernum, Governor, buf);
      sprintf(buf, "%7.2f   %7.1f,%7.1f   %8.1f,%8.1f\n", gravity(p),
              p->xpos + Stars[where.snum]->xpos,
              p->ypos + Stars[where.snum]->ypos, p->xpos, p->ypos);
      notify(Playernum, Governor, buf);
      sprintf(buf, "======== Planetary conditions: ========\n");
      notify(Playernum, Governor, buf);
      sprintf(buf, "atmosphere concentrations:\n");
      notify(Playernum, Governor, buf);
      sprintf(buf, "     methane %02d%%(%02d%%)     oxygen %02d%%(%02d%%)\n",
              p->conditions[METHANE], Race->conditions[METHANE],
              p->conditions[OXYGEN], Race->conditions[OXYGEN]);
      notify(Playernum, Governor, buf);
      sprintf(buf,
              "         CO2 %02d%%(%02d%%)   hydrogen %02d%%(%02d%%)      "
              "temperature: %3d (%3d)\n",
              p->conditions[CO2], Race->conditions[CO2],
              p->conditions[HYDROGEN], Race->conditions[HYDROGEN],
              p->conditions[TEMP], Race->conditions[TEMP]);
      notify(Playernum, Governor, buf);
      sprintf(buf,
              "    nitrogen %02d%%(%02d%%)     sulfur %02d%%(%02d%%)      "
              "     normal: %3d\n",
              p->conditions[NITROGEN], Race->conditions[NITROGEN],
              p->conditions[SULFUR], Race->conditions[SULFUR],
              p->conditions[RTEMP]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "      helium %02d%%(%02d%%)      other %02d%%(%02d%%)\n",
              p->conditions[HELIUM], Race->conditions[HELIUM],
              p->conditions[OTHER], Race->conditions[OTHER]);
      notify(Playernum, Governor, buf);
      if ((tindex = p->conditions[TOXIC] / 10) < 0)
        tindex = 0;
      else if (tindex > 10)
        tindex = 11;
      sprintf(buf, "                     Toxicity: %d%% (%s)\n",
              p->conditions[TOXIC], Tox[tindex]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "Total planetary compatibility: %.2f%%\n",
              compatibility(p, Race));
      notify(Playernum, Governor, buf);

      getsmap(Smap, p);

      crystal_count = avg_fert = avg_resource = 0;
      for (lowx = 0; lowx < p->Maxx; lowx++)
        for (lowy = 0; lowy < p->Maxy; lowy++) {
          s = &Sector(*p, lowx, lowy);
          avg_fert += s->fert;
          avg_resource += s->resource;
          if (Race->discoveries[D_CRYSTAL] || Race->God)
            crystal_count += !!s->crystals;
        }
      sprintf(buf, "%29s: %d\n%29s: %d\n%29s: %d\n", "Average fertility",
              avg_fert / (p->Maxx * p->Maxy), "Average resource",
              avg_resource / (p->Maxx * p->Maxy), "Crystal sectors",
              crystal_count);
      notify(Playernum, Governor, buf);
      if (LIMITED_RESOURCES) {
        sprintf(buf, "%29s: %ld\n", "Total resource deposits",
                p->total_resources);
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, "fuel_stock  resource_stock dest_pot.   %s    ^%s\n",
              Race->Metamorph ? "biomass" : "popltn",
              Race->Metamorph ? "biomass" : "popltn");
      notify(Playernum, Governor, buf);
      sprintf(buf, "%10u  %14u %9u  %7lu%11lu\n", p->info[Playernum - 1].fuel,
              p->info[Playernum - 1].resource, p->info[Playernum - 1].destruct,
              p->popn, p->maxpopn);
      notify(Playernum, Governor, buf);
      if (p->slaved_to) {
        sprintf(buf, "This planet ENSLAVED to player %d!\n", p->slaved_to);
        notify(Playernum, Governor, buf);
      }
    }
    free((char *)p);
  } else if (where.level == LEVEL_STAR) {
    sprintf(buf, "Star %s\n", Stars[where.snum]->name);
    notify(Playernum, Governor, buf);
    sprintf(buf, "locn: %f,%f\n", Stars[where.snum]->xpos,
            Stars[where.snum]->ypos);
    notify(Playernum, Governor, buf);
    if (Race->God) {
      for (i = 0; i < Stars[where.snum]->numplanets; i++) {
        getplanet(&p, (int)where.snum, i);
        sprintf(buf, "%8d \"%s\"\n", p->sectormappos,
                Stars[where.snum]->pnames[i]);
        notify(Playernum, Governor, buf);
        free((char *)p);
      }
    }
    sprintf(buf, "Gravity: %.2f\tInstability: ", Stars[where.snum]->gravity);
    notify(Playernum, Governor, buf);

    if (Race->tech >= TECH_SEE_STABILITY || Race->God) {
      sprintf(buf, "%d%% (%s)\n", Stars[where.snum]->stability,
              Stars[where.snum]->stability < 20
                  ? "stable"
                  : Stars[where.snum]->stability < 40
                        ? "unstable"
                        : Stars[where.snum]->stability < 60
                              ? "dangerous"
                              : Stars[where.snum]->stability < 100
                                    ? "WARNING! Nova iminent!"
                                    : "undergoing nova");
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "(cannot determine)\n");
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "temperature class (1->10) %d\n",
            Stars[where.snum]->temperature);
    notify(Playernum, Governor, buf);
    sprintf(buf, "%d planets are ", Stars[where.snum]->numplanets);
    notify(Playernum, Governor, buf);
    for (x2 = 0; x2 < Stars[where.snum]->numplanets; x2++) {
      sprintf(buf, "%s ", Stars[where.snum]->pnames[x2]);
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "\n");
    notify(Playernum, Governor, buf);
  } else if (where.level == LEVEL_UNIV) {
    sprintf(buf, "It's just _there_, you know?\n");
    notify(Playernum, Governor, buf);
  } else {
    sprintf(buf, "Illegal scope.\n");
    notify(Playernum, Governor, buf);
  }
} /* end survey */

void repair(const command_t &argv, const player_t Playernum,
            const governor_t Governor) {
  int lowx, hix, lowy, hiy, x2, sectors, cost;
  sectortype *s;
  planettype *p;
  placetype where;

  /* general code -- jpd -- */
  if (argn == 1) { /* no args */
    where.level = Dir[Playernum - 1][Governor].level;
    where.snum = Dir[Playernum - 1][Governor].snum;
    where.pnum = Dir[Playernum - 1][Governor].pnum;
  } else {
    /* repairing a sector */
    if (isdigit(args[1][0]) && index(args[1], ',') != NULL) {
      if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
        sprintf(buf, "There are no sectors here.\n");
        notify(Playernum, Governor, buf);
        return;
      } else {
        where.level = LEVEL_PLAN;
        where.snum = Dir[Playernum - 1][Governor].snum;
        where.pnum = Dir[Playernum - 1][Governor].pnum;
      }
    } else {
      where = Getplace(Playernum, Governor, args[1], 0);
      if (where.err || where.level == LEVEL_SHIP) return;
    }
  }

  if (where.level == LEVEL_PLAN) {
    getplanet(&p, (int)where.snum, (int)where.pnum);
    if (!p->info[Playernum - 1].numsectsowned) {
      notify(Playernum, Governor,
             "You don't own any sectors on this planet.\n");
      free((char *)p);
      return;
    }
    getsmap(Smap, p);
    if (isdigit(args[1][0]) && index(args[1], ',') != NULL) {
      get4args(args[1], &x2, &hix, &lowy, &hiy);
      /* ^^^ translate from lowx:hix,lowy:hiy */
      x2 = std::max(0, x2);
      hix = std::min(hix, p->Maxx - 1);
      lowy = std::max(0, lowy);
      hiy = std::min(hiy, p->Maxy - 1);
    } else {
      /* repair entire planet */
      x2 = 0;
      hix = p->Maxx - 1;
      lowy = 0;
      hiy = p->Maxy - 1;
    }
    sectors = 0;
    cost = 0;

    for (; lowy <= hiy; lowy++)
      for (lowx = x2; lowx <= hix; lowx++) {
        if (p->info[Playernum - 1].resource >= SECTOR_REPAIR_COST) {
          s = &Sector(*p, lowx, lowy);
          if (s->condition == WASTED && (s->owner == Playernum || !s->owner)) {
            s->condition = s->type;
            s->fert = std::min(100, s->fert + 20);
            p->info[Playernum - 1].resource -= SECTOR_REPAIR_COST;
            cost += SECTOR_REPAIR_COST;
            sectors += 1;
            putsector(s, p, lowx, lowy);
          }
        }
      }
    putplanet(p, (int)where.snum, (int)where.pnum);
    free((char *)p);

    sprintf(buf, "%d sectors repaired at a cost of %d resources.\n", sectors,
            cost);
    notify(Playernum, Governor, buf);
  } else {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
  }
}
