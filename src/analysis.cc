// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "analysis.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffers.h"
#include "getplace.h"
#include "races.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"
#include "max.h"
#include "files_shl.h"
#include "GB_server.h"

#define CARE 5
struct anal_sect {
  int x, y;
  int value;
  int des;
};

static void do_analysis(int, int, int, int, int, starnum_t, planetnum_t);
static void Insert(int, struct anal_sect[], int, int, int, int);
static void PrintTop(int, int, struct anal_sect[], const char *);

void analysis(int Playernum, int Governor, int APcount) {
  int sector_type = -1; /* -1 does analysis on all types */
  placetype where;      /* otherwise on specific type */
  int i;
  int do_player = -1;
  char *p;
  int mode = 1; /* does top five. 0 does low five */

  i = 1;
  do {
    where.level = Dir[Playernum - 1][Governor].level;
    where.snum = Dir[Playernum - 1][Governor].snum;
    where.pnum = Dir[Playernum - 1][Governor].pnum;

    p = args[1];
    if (*p == '-') /*  Must use 'd' to do an analysis on */
    {              /*  desert sectors to avoid confusion */
      p++;         /*  with the '-' for the mode type    */
      i++;
      mode = 0;
    }
    switch (*p) {
    case CHAR_SEA:
      sector_type = SEA;
      break;
    case CHAR_LAND:
      sector_type = LAND;
      break;
    case CHAR_MOUNT:
      sector_type = MOUNT;
      break;
    case CHAR_GAS:
      sector_type = GAS;
      break;
    case CHAR_ICE:
      sector_type = ICE;
      break;
    case CHAR_FOREST:
      sector_type = FOREST;
      break;
    case 'd':
      sector_type = DESERT;
      break;
    case CHAR_PLATED:
      sector_type = PLATED;
      break;
    case CHAR_WASTED:
      sector_type = WASTED;
      break;
    }
    if (sector_type != -1 && mode == 1) {
      i++;
    }

    p = args[i];
    if (isdigit(*p)) {
      do_player = atoi(p);
      if (do_player > Num_races) {
        notify(Playernum, Governor, "No such player #.\n");
        return;
      }
      where.level = Dir[Playernum - 1][Governor].level;
      where.snum = Dir[Playernum - 1][Governor].snum;
      where.pnum = Dir[Playernum - 1][Governor].pnum;
      i++;
    }
    p = args[i];
    if (i < argn && (isalpha(*p) || *p == '/')) {
      where = Getplace(Playernum, Governor, args[i], 0);
      if (where.err)
        continue;
    }

    switch (where.level) {
    case LEVEL_UNIV:
    case LEVEL_SHIP:
      notify(Playernum, Governor, "You can only analyze planets.\n");
      break;
    case LEVEL_PLAN:
      do_analysis(Playernum, Governor, do_player, mode, sector_type, where.snum,
                  where.pnum);
      break;
    case LEVEL_STAR:
      for (planetnum_t pnum = 0; pnum < Stars[where.snum]->numplanets; pnum++)
        do_analysis(Playernum, Governor, do_player, mode, sector_type,
                    where.snum, pnum);
      break;
    }
  } while (0);
  return;
}

static void do_analysis(int Playernum, int Governor, int ThisPlayer, int mode,
                        int sector_type, starnum_t Starnum,
                        planetnum_t Planetnum) {
  planettype *planet;
  sectortype *sect;
  racetype *Race;
  int x, y;
  int p;
  int i;
  double compat;
  struct anal_sect Res[CARE], Eff[CARE], Frt[CARE], Mob[CARE];
  struct anal_sect Troops[CARE], Popn[CARE], mPopn[CARE];
  int TotalCrys, PlayCrys[MAXPLAYERS + 1];
  int TotalTroops, PlayTroops[MAXPLAYERS + 1];
  int TotalPopn, PlayPopn[MAXPLAYERS + 1];
  int TotalMob, PlayMob[MAXPLAYERS + 1];
  int TotalEff, PlayEff[MAXPLAYERS + 1];
  int TotalRes, PlayRes[MAXPLAYERS + 1];
  int TotalSect, PlaySect[MAXPLAYERS + 1][WASTED + 1];
  int PlayTSect[MAXPLAYERS + 1];
  int TotalWasted, WastedSect[MAXPLAYERS + 1];
  int Sect[WASTED + 1];
  static char SectTypes[] = { CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,
                              CHAR_GAS,    CHAR_ICE,    CHAR_FOREST,
                              CHAR_DESERT, CHAR_PLATED, CHAR_WASTED };

  for (i = 0; i < CARE; i++)
    Res[i].value = Eff[i].value = Frt[i].value = Mob[i].value =
        Troops[i].value = Popn[i].value = mPopn[i].value = -1;

  TotalWasted = TotalCrys = TotalPopn = TotalMob = TotalTroops = TotalEff =
      TotalRes = TotalSect = 0;
  for (p = 0; p <= Num_races; p++) {
    PlayTroops[p] = PlayPopn[p] = PlayMob[p] = PlayEff[p] = PlayCrys[p] =
        PlayRes[p] = PlayTSect[p] = 0;
    WastedSect[p] = 0;
    for (i = 0; i <= WASTED; i++)
      PlaySect[p][i] = 0;
  }

  for (i = 0; i <= WASTED; i++)
    Sect[i] = 0;

  Race = races[Playernum - 1];
  getplanet(&planet, Starnum, Planetnum);

  if (!planet->info[Playernum - 1].explored) {
    free((char *)planet);
    return;
  }
  getsmap(Smap, planet);

  compat = compatibility(planet, Race);

  TotalSect = planet->Maxx * planet->Maxy;
  for (x = planet->Maxx - 1; x >= 0; x--) {
    for (y = planet->Maxy - 1; y >= 0; y--) {
      sect = &Sector(*planet, x, y);
      p = sect->owner;

      PlayEff[p] += sect->eff;
      PlayMob[p] += sect->mobilization;
      PlayRes[p] += sect->resource;
      PlayPopn[p] += sect->popn;
      PlayTroops[p] += sect->troops;
      PlaySect[p][sect->condition]++;
      PlayTSect[p]++;
      TotalEff += sect->eff;
      TotalMob += sect->mobilization;
      TotalRes += sect->resource;
      TotalPopn += sect->popn;
      TotalTroops += sect->troops;
      Sect[sect->condition]++;

      if (sect->condition == WASTED) {
        WastedSect[p]++;
        TotalWasted++;
      }
      if (sect->crystals && Race->tech >= TECH_CRYSTAL) {
        PlayCrys[p]++;
        TotalCrys++;
      }

      if (sector_type == -1 || sector_type == sect->condition) {
        if (ThisPlayer < 0 || ThisPlayer == p) {
          Insert(mode, Res, x, y, sect->condition, (int)sect->resource);
          Insert(mode, Eff, x, y, sect->condition, (int)sect->eff);
          Insert(mode, Mob, x, y, sect->condition, (int)sect->mobilization);
          Insert(mode, Frt, x, y, sect->condition, (int)sect->fert);
          Insert(mode, Popn, x, y, sect->condition, (int)sect->popn);
          Insert(mode, Troops, x, y, sect->condition, (int)sect->troops);
          Insert(
              mode, mPopn, x, y, sect->condition,
              maxsupport(Race, sect, compat, (int)planet->conditions[TOXIC]));
        }
      }
    }
  }

  sprintf(buf, "\nAnalysis of /%s/%s:\n", Stars[Starnum]->name,
          Stars[Starnum]->pnames[Planetnum]);
  notify(Playernum, Governor, buf);
  sprintf(buf, "%s %d", (mode ? "Highest" : "Lowest"), CARE);
  switch (sector_type) {
  case -1:
    sprintf(buf, "%s of all", buf);
    break;
  case SEA:
    sprintf(buf, "%s Ocean", buf);
    break;
  case LAND:
    sprintf(buf, "%s Land", buf);
    break;
  case MOUNT:
    sprintf(buf, "%s Mountain", buf);
    break;
  case GAS:
    sprintf(buf, "%s Gas", buf);
    break;
  case ICE:
    sprintf(buf, "%s Ice", buf);
    break;
  case FOREST:
    sprintf(buf, "%s Forest", buf);
    break;
  case DESERT:
    sprintf(buf, "%s Desert", buf);
    break;
  case PLATED:
    sprintf(buf, "%s Plated", buf);
    break;
  case WASTED:
    sprintf(buf, "%s Wasted", buf);
    break;
  }
  notify(Playernum, Governor, buf);
  if (ThisPlayer < 0)
    sprintf(buf, " sectors.\n");
  else if (ThisPlayer == 0)
    sprintf(buf, " sectors that are unoccupied.\n");
  else
    sprintf(buf, " sectors owned by %d.\n", ThisPlayer);
  notify(Playernum, Governor, buf);

  PrintTop(Playernum, Governor, Troops, "Troops");
  PrintTop(Playernum, Governor, Res, "Res");
  PrintTop(Playernum, Governor, Eff, "Eff");
  PrintTop(Playernum, Governor, Frt, "Frt");
  PrintTop(Playernum, Governor, Mob, "Mob");
  PrintTop(Playernum, Governor, Popn, "Popn");
  PrintTop(Playernum, Governor, mPopn, "^Popn");

  notify(Playernum, Governor, "\n");
  sprintf(buf, "%2s %3s %7s %6s %5s %5s %5s %2s", "Pl", "sec", "popn", "troops",
          "a.eff", "a.mob", "res", "x");
  notify(Playernum, Governor, buf);

  for (i = 0; i <= WASTED; i++) {
    sprintf(buf, "%4c", SectTypes[i]);
    notify(Playernum, Governor, buf);
  }
  notify(Playernum, Governor, "\n----------------------------------------------"
                              "---------------------------------\n");
  for (p = 0; p <= Num_races; p++)
    if (PlayTSect[p] != 0) {
      sprintf(buf, "%2d %3d %7d %6d %5.1lf %5.1lf %5d %2d", p, PlayTSect[p],
              PlayPopn[p], PlayTroops[p], (double)PlayEff[p] / PlayTSect[p],
              (double)PlayMob[p] / PlayTSect[p], PlayRes[p], PlayCrys[p]);
      notify(Playernum, Governor, buf);
      for (i = 0; i <= WASTED; i++) {
        sprintf(buf, "%4d", PlaySect[p][i]);
        notify(Playernum, Governor, buf);
      }
      notify(Playernum, Governor, "\n");
    }
  notify(Playernum, Governor, "------------------------------------------------"
                              "-------------------------------\n");
  sprintf(buf, "%2s %3d %7d %6d %5.1lf %5.1lf %5d %2d", "Tl", TotalSect,
          TotalPopn, TotalTroops, (double)TotalEff / TotalSect,
          (double)TotalMob / TotalSect, TotalRes, TotalCrys);
  notify(Playernum, Governor, buf);
  for (i = 0; i <= WASTED; i++) {
    sprintf(buf, "%4d", Sect[i]);
    notify(Playernum, Governor, buf);
  }
  notify(Playernum, Governor, "\n");
  free((char *)planet);
}

static void Insert(int mode, struct anal_sect arr[], int x, int y, int des,
                   int value) {
  int i, j;

  for (i = 0; i < CARE; i++)
    if ((mode && arr[i].value < value) ||
        (!mode && (arr[i].value > value || arr[i].value == -1))) {
      for (j = CARE - 1; j > i; j--)
        arr[j] = arr[j - 1];
      arr[i].value = value;
      arr[i].x = x;
      arr[i].y = y;
      arr[i].des = des;
      return;
    }
}

static void PrintTop(int Playernum, int Governor, struct anal_sect arr[],
                     const char *name) {
  int i;

  sprintf(buf, "%8s:", name);
  notify(Playernum, Governor, buf);
  for (i = 0; i < CARE && arr[i].value != -1; i++) {
    sprintf(buf, "%5d%c(%2d,%2d)", arr[i].value, Dessymbols[arr[i].des],
            arr[i].x, arr[i].y);
    notify(Playernum, Governor, buf);
  }
  notify(Playernum, Governor, "\n");
}
