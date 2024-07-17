// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* survey.c -- print out survey for planets */

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/csp.h"
#include "gb/csp_types.h"
#include "gb/files_shl.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/place.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

module commands;

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

void survey(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int lowx;
  int hix;
  int lowy;
  int hiy;
  int x2;
  char d;
  char sect_char;
  int tindex;
  double compat;
  int avg_fert;
  int avg_resource;
  int crystal_count;
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
  int i;

  int mode;
  if (argv[0] == "survey")
    mode = 0;
  else
    mode = 1;

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level, g.snum, g.pnum);
  } else {
    /* they are surveying a sector */
    if ((isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) ||
        ((argv[1][0] == '-') && (all = 1))) {
      if (g.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "There are no sectors here.\n";
        return;
      }
      where = std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum, g.pnum);

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return;
    }
  }

  auto &race = races[Playernum - 1];

  if (where->level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(where->snum, where->pnum);

    compat = p.compatibility(race);

    if ((isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) ||
        all) {
      auto smap = getsmap(p);

      if (!all) {
        get4args(argv[1].c_str(), &x2, &hix, &lowy, &hiy);
        /* ^^^ translate from lowx:hix,lowy:hiy */
        x2 = std::max(0, x2);
        hix = std::min(hix, p.Maxx - 1);
        lowy = std::max(0, lowy);
        hiy = std::min(hiy, p.Maxy - 1);
      } else {
        x2 = 0;
        hix = p.Maxx - 1;
        lowy = 0;
        hiy = p.Maxy - 1;
      }

      if (!mode) {
        g.out << " x,y cond/type  owner race eff mob frt  res  mil popn "
                 "^popn xtals\n";
      }
      if (mode) {
        if (all) {
          sprintf(buf, "%c %d %d %d %s %s %lu %d %d %ld %ld %d %.2f %d\n",
                  CSP_CLIENT, CSP_SURVEY_INTRO, p.Maxx, p.Maxy,
                  stars[where->snum].name,
                  stars[where->snum].pnames[where->pnum],
                  p.info[Playernum - 1].resource, p.info[Playernum - 1].fuel,
                  p.info[Playernum - 1].destruct, p.popn, p.maxpopn,
                  p.conditions[TOXIC], p.compatibility(race), p.slaved_to);
          notify(Playernum, Governor, buf);
        }
        bzero((struct shipstuff *)shiplocs, sizeof(shiplocs));
        inhere = p.info[Playernum - 1].numsectsowned;
        shiplist = p.ships;
        while (shiplist) {
          auto shipa = getship(shiplist);
          if (shipa->owner == Playernum &&
              (shipa->popn || (shipa->type == ShipType::OTYPE_PROBE)))
            inhere = 1;
          if (shipa->alive && landed(*shipa) &&
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
        }
      }
      for (; lowy <= hiy; lowy++)
        for (lowx = x2; lowx <= hix; lowx++) {
          auto &s = smap.get(lowx, lowy);
          /* if (s->owner==Playernum) */
          if (!mode) {
            sprintf(buf, "%2d,%-2d ", lowx, lowy);
            notify(Playernum, Governor, buf);
            if ((d = desshow(Playernum, Governor, race, s)) == CHAR_CLOAKED) {
              sprintf(buf, "?  (    ?    )\n");
              notify(Playernum, Governor, buf);
            } else {
              sprintf(buf, " %c   %c   %6u%5u%4u%4u%4u%5lu%5lu%5lu%6ld%s\n",
                      Dessymbols[s.condition], Dessymbols[s.type], s.owner,
                      s.race, s.eff, s.mobilization, s.fert, s.resource,
                      s.troops, s.popn,
                      maxsupport(race, s, compat, p.conditions[TOXIC]),
                      ((s.crystals && (race.discoveries[D_CRYSTAL] || race.God))
                           ? " yes"
                           : " "));
              notify(Playernum, Governor, buf);
            }
          } else { /* mode */
            switch (s.condition) {
              case SectorType::SEC_SEA:
                sect_char = CHAR_SEA;
                break;
              case SectorType::SEC_LAND:
                sect_char = CHAR_LAND;
                break;
              case SectorType::SEC_MOUNT:
                sect_char = CHAR_MOUNT;
                break;
              case SectorType::SEC_GAS:
                sect_char = CHAR_GAS;
                break;
              case SectorType::SEC_PLATED:
                sect_char = CHAR_PLATED;
                break;
              case SectorType::SEC_ICE:
                sect_char = CHAR_ICE;
                break;
              case SectorType::SEC_DESERT:
                sect_char = CHAR_DESERT;
                break;
              case SectorType::SEC_FOREST:
                sect_char = CHAR_FOREST;
                break;
              default:
                sect_char = '?';
                break;
            }
            sprintf(
                buf, "%c %d %d %d %c %c %d %u %u %u %u %d %lu %lu %lu %ld",
                CSP_CLIENT, CSP_SURVEY_SECTOR, lowx, lowy, sect_char,
                desshow(Playernum, Governor, race, s),
                ((s.condition == SectorType::SEC_WASTED) ? 1 : 0), s.owner,
                s.eff, s.fert, s.mobilization,
                ((s.crystals && (race.discoveries[D_CRYSTAL] || race.God)) ? 1
                                                                           : 0),
                s.resource, s.popn, s.troops,
                maxsupport(race, s, compat, p.conditions[TOXIC]));
            notify(Playernum, Governor, buf);

            if (shiplocs[lowx][lowy].pos && inhere) {
              g.out << ";";
              for (i = 0; i < shiplocs[lowx][lowy].pos; i++) {
                sprintf(buf, " %d %c %u;",
                        shiplocs[lowx][lowy].shipstuffs[i].shipno,
                        shiplocs[lowx][lowy].shipstuffs[i].ltr,
                        shiplocs[lowx][lowy].shipstuffs[i].owner);
                notify(Playernum, Governor, buf);
              }
            }
            g.out << "\n";
          }
        }
      if (mode) {
        sprintf(buf, "%c %d\n", CSP_CLIENT, CSP_SURVEY_END);
        notify(Playernum, Governor, buf);
      }
    } else {
      /* survey of planet */
      sprintf(buf, "%s:\n", stars[where->snum].pnames[where->pnum]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "gravity   x,y absolute     x,y relative to %s\n",
              stars[where->snum].name);
      notify(Playernum, Governor, buf);
      sprintf(buf, "%7.2f   %7.1f,%7.1f   %8.1f,%8.1f\n", p.gravity(),
              p.xpos + stars[where->snum].xpos,
              p.ypos + stars[where->snum].ypos, p.xpos, p.ypos);
      notify(Playernum, Governor, buf);
      g.out << "======== Planetary conditions: ========\n";
      g.out << "atmosphere concentrations:\n";
      sprintf(buf, "     methane %02d%%(%02d%%)     oxygen %02d%%(%02d%%)\n",
              p.conditions[METHANE], race.conditions[METHANE],
              p.conditions[OXYGEN], race.conditions[OXYGEN]);
      notify(Playernum, Governor, buf);
      sprintf(buf,
              "         CO2 %02d%%(%02d%%)   hydrogen %02d%%(%02d%%)      "
              "temperature: %3d (%3d)\n",
              p.conditions[CO2], race.conditions[CO2], p.conditions[HYDROGEN],
              race.conditions[HYDROGEN], p.conditions[TEMP],
              race.conditions[TEMP]);
      notify(Playernum, Governor, buf);
      sprintf(buf,
              "    nitrogen %02d%%(%02d%%)     sulfur %02d%%(%02d%%)      "
              "     normal: %3d\n",
              p.conditions[NITROGEN], race.conditions[NITROGEN],
              p.conditions[SULFUR], race.conditions[SULFUR],
              p.conditions[RTEMP]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "      helium %02d%%(%02d%%)      other %02d%%(%02d%%)\n",
              p.conditions[HELIUM], race.conditions[HELIUM],
              p.conditions[OTHER], race.conditions[OTHER]);
      notify(Playernum, Governor, buf);
      if ((tindex = p.conditions[TOXIC] / 10) < 0)
        tindex = 0;
      else if (tindex > 10)
        tindex = 11;
      sprintf(buf, "                     Toxicity: %d%% (%s)\n",
              p.conditions[TOXIC], Tox[tindex]);
      notify(Playernum, Governor, buf);
      sprintf(buf, "Total planetary compatibility: %.2f%%\n",
              p.compatibility(race));
      notify(Playernum, Governor, buf);

      auto smap = getsmap(p);

      crystal_count = avg_fert = avg_resource = 0;
      for (lowx = 0; lowx < p.Maxx; lowx++)
        for (lowy = 0; lowy < p.Maxy; lowy++) {
          auto &s = smap.get(lowx, lowy);
          avg_fert += s.fert;
          avg_resource += s.resource;
          if (race.discoveries[D_CRYSTAL] || race.God)
            crystal_count += !!s.crystals;
        }
      sprintf(buf, "%29s: %d\n%29s: %d\n%29s: %d\n", "Average fertility",
              avg_fert / (p.Maxx * p.Maxy), "Average resource",
              avg_resource / (p.Maxx * p.Maxy), "Crystal sectors",
              crystal_count);
      notify(Playernum, Governor, buf);
      if (LIMITED_RESOURCES) {
        sprintf(buf, "%29s: %ld\n", "Total resource deposits",
                p.total_resources);
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, "fuel_stock  resource_stock dest_pot.   %s    ^%s\n",
              race.Metamorph ? "biomass" : "popltn",
              race.Metamorph ? "biomass" : "popltn");
      notify(Playernum, Governor, buf);
      sprintf(buf, "%10u  %14lu %9u  %7lu%11lu\n", p.info[Playernum - 1].fuel,
              p.info[Playernum - 1].resource, p.info[Playernum - 1].destruct,
              p.popn, p.maxpopn);
      notify(Playernum, Governor, buf);
      if (p.slaved_to) {
        sprintf(buf, "This planet ENSLAVED to player %d!\n", p.slaved_to);
        notify(Playernum, Governor, buf);
      }
    }
  } else if (where->level == ScopeLevel::LEVEL_STAR) {
    sprintf(buf, "Star %s\n", stars[where->snum].name);
    notify(Playernum, Governor, buf);
    sprintf(buf, "locn: %f,%f\n", stars[where->snum].xpos,
            stars[where->snum].ypos);
    notify(Playernum, Governor, buf);
    if (race.God) {
      for (i = 0; i < stars[where->snum].numplanets; i++) {
        sprintf(buf, " \"%s\"\n", stars[where->snum].pnames[i]);
        notify(Playernum, Governor, buf);
      }
    }
    sprintf(buf, "Gravity: %.2f\tInstability: ", stars[where->snum].gravity);
    notify(Playernum, Governor, buf);

    if (race.tech >= TECH_SEE_STABILITY || race.God) {
      sprintf(buf, "%d%% (%s)\n", stars[where->snum].stability,
              stars[where->snum].stability < 20    ? "stable"
              : stars[where->snum].stability < 40  ? "unstable"
              : stars[where->snum].stability < 60  ? "dangerous"
              : stars[where->snum].stability < 100 ? "WARNING! Nova iminent!"
                                                   : "undergoing nova");
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "(cannot determine)\n");
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "temperature class (1->10) %d\n",
            stars[where->snum].temperature);
    notify(Playernum, Governor, buf);
    sprintf(buf, "%d planets are ", stars[where->snum].numplanets);
    notify(Playernum, Governor, buf);
    for (x2 = 0; x2 < stars[where->snum].numplanets; x2++) {
      sprintf(buf, "%s ", stars[where->snum].pnames[x2]);
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "\n");
    notify(Playernum, Governor, buf);
  } else if (where->level == ScopeLevel::LEVEL_UNIV) {
    g.out << "It's just _there_, you know?\n";
  } else {
    g.out << "Illegal scope.\n";
  }
} /* end survey */
