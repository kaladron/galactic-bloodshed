// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/analysis.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/getplace.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/vars.h"

static constexpr int CARE = 5;

struct anal_sect {
  unsigned int x, y;
  unsigned int des;
  resource_t value;
};

namespace {
enum class Mode { top_five, bottom_five };

void insert(Mode mode, std::array<struct anal_sect, CARE> arr, anal_sect in) {
  for (int i = 0; i < CARE; i++)
    if ((mode == Mode::top_five && arr[i].value < in.value) ||
        (mode == Mode::bottom_five &&
         (arr[i].value > in.value || arr[i].value == -1))) {
      for (int j = CARE - 1; j > i; j--) arr[j] = arr[j - 1];
      arr[i] = in;
      return;
    }
}

void PrintTop(GameObj &g, const std::array<struct anal_sect, CARE> arr,
              const std::string name) {
  sprintf(buf, "%8s:", name.c_str());
  notify(g.player, g.governor, buf);

  for (const auto &as : arr) {
    if (as.value == -1) continue;
    sprintf(buf, "%5ld%c(%2d,%2d)", as.value, Dessymbols[as.des], as.x, as.y);
    notify(g.player, g.governor, buf);
  }
  g.out << "\n";
}
}  // namespace

static void do_analysis(GameObj &, int, Mode, int, starnum_t, planetnum_t);

void analysis(const command_t &argv, GameObj &g) {
  int sector_type = -1; /* -1 does analysis on all types */
  int do_player = -1;
  auto mode = Mode::top_five;

  size_t i = 1;
  do {
    auto where = std::make_unique<Place>(g.level, g.snum, g.pnum);

    const char *p = argv[1].c_str();
    if (*p == '-') /*  Must use 'd' to do an analysis on */
    {              /*  desert sectors to avoid confusion */
      p++;         /*  with the '-' for the mode type    */
      i++;
      mode = Mode::bottom_five;
    }
    switch (*p) {
      case CHAR_SEA:
        sector_type = SectorType::SEC_SEA;
        break;
      case CHAR_LAND:
        sector_type = SectorType::SEC_LAND;
        break;
      case CHAR_MOUNT:
        sector_type = SectorType::SEC_MOUNT;
        break;
      case CHAR_GAS:
        sector_type = SectorType::SEC_GAS;
        break;
      case CHAR_ICE:
        sector_type = SectorType::SEC_ICE;
        break;
      case CHAR_FOREST:
        sector_type = SectorType::SEC_FOREST;
        break;
      case 'd':
        sector_type = SectorType::SEC_DESERT;
        break;
      case CHAR_PLATED:
        sector_type = SectorType::SEC_PLATED;
        break;
      case CHAR_WASTED:
        sector_type = SectorType::SEC_WASTED;
        break;
    }
    if (sector_type != -1 && mode == Mode::top_five) {
      i++;
    }

    p = argv[i].c_str();
    if (isdigit(*p)) {
      do_player = atoi(p);
      if (do_player > Num_races) {
        g.out << "No such player #.\n";
        return;
      }
      where = std::make_unique<Place>(g.level, g.snum, g.pnum);
      i++;
    }
    p = argv[i].c_str();
    if (i < argv.size() && (isalpha(*p) || *p == '/')) {
      where = std::make_unique<Place>(g, argv[i]);
      if (where->err) continue;
    }

    switch (where->level) {
      case ScopeLevel::LEVEL_UNIV:
        [[fallthrough]];
      case ScopeLevel::LEVEL_SHIP:
        g.out << "You can only analyze planets.\n";
        break;
      case ScopeLevel::LEVEL_PLAN:
        do_analysis(g, do_player, mode, sector_type, where->snum, where->pnum);
        break;
      case ScopeLevel::LEVEL_STAR:
        for (planetnum_t pnum = 0; pnum < stars[where->snum].numplanets; pnum++)
          do_analysis(g, do_player, mode, sector_type, where->snum, pnum);
        break;
    }
  } while (false);
}

static void do_analysis(GameObj &g, int ThisPlayer, Mode mode, int sector_type,
                        starnum_t Starnum, planetnum_t Planetnum) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  std::array<struct anal_sect, CARE> Res;
  std::array<struct anal_sect, CARE> Eff;
  std::array<struct anal_sect, CARE> Frt;
  std::array<struct anal_sect, CARE> Mob;
  std::array<struct anal_sect, CARE> Troops;
  std::array<struct anal_sect, CARE> Popn;
  std::array<struct anal_sect, CARE> mPopn;
  int TotalCrys;
  int PlayCrys[MAXPLAYERS + 1];
  int TotalTroops;
  int PlayTroops[MAXPLAYERS + 1];
  int TotalPopn;
  int PlayPopn[MAXPLAYERS + 1];
  int TotalMob;
  int PlayMob[MAXPLAYERS + 1];
  int TotalEff;
  int PlayEff[MAXPLAYERS + 1];
  int TotalRes;
  int PlayRes[MAXPLAYERS + 1];
  int TotalSect;
  int PlaySect[MAXPLAYERS + 1][SectorType::SEC_WASTED + 1];
  int PlayTSect[MAXPLAYERS + 1];
  int TotalWasted;
  int WastedSect[MAXPLAYERS + 1];
  int Sect[SectorType::SEC_WASTED + 1];
  static char SectTypes[] = {CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,
                             CHAR_GAS,    CHAR_ICE,    CHAR_FOREST,
                             CHAR_DESERT, CHAR_PLATED, CHAR_WASTED};

  for (int i = 0; i < CARE; i++)
    Res[i].value = Eff[i].value = Frt[i].value = Mob[i].value =
        Troops[i].value = Popn[i].value = mPopn[i].value = -1;

  TotalWasted = TotalCrys = TotalPopn = TotalMob = TotalTroops = TotalEff =
      TotalRes = TotalSect = 0;
  for (int p = 0; p <= Num_races; p++) {
    PlayTroops[p] = PlayPopn[p] = PlayMob[p] = PlayEff[p] = PlayCrys[p] =
        PlayRes[p] = PlayTSect[p] = 0;
    WastedSect[p] = 0;
    for (int i = 0; i <= SectorType::SEC_WASTED; i++) PlaySect[p][i] = 0;
  }

  for (int i = 0; i <= SectorType::SEC_WASTED; i++) Sect[i] = 0;

  auto &race = races[Playernum - 1];
  const auto planet = getplanet(Starnum, Planetnum);

  if (!planet.info[Playernum - 1].explored) {
    return;
  }

  TotalSect = planet.Maxx * planet.Maxy;

  for (auto smap = getsmap(planet); auto &sect : smap) {
    auto p = sect.owner;

    PlayEff[p] += sect.eff;
    PlayMob[p] += sect.mobilization;
    PlayRes[p] += sect.resource;
    PlayPopn[p] += sect.popn;
    PlayTroops[p] += sect.troops;
    PlaySect[p][sect.condition]++;
    PlayTSect[p]++;
    TotalEff += sect.eff;
    TotalMob += sect.mobilization;
    TotalRes += sect.resource;
    TotalPopn += sect.popn;
    TotalTroops += sect.troops;
    Sect[sect.condition]++;

    if (sect.condition == SectorType::SEC_WASTED) {
      WastedSect[p]++;
      TotalWasted++;
    }
    if (sect.crystals && race.tech >= TECH_CRYSTAL) {
      PlayCrys[p]++;
      TotalCrys++;
    }

    if (sector_type == -1 || sector_type == sect.condition) {
      if (ThisPlayer < 0 || ThisPlayer == p) {
        insert(mode, Res,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.resource});
        insert(mode, Eff,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.eff});
        insert(mode, Mob,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.mobilization});
        insert(mode, Frt,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.fert});
        insert(mode, Popn,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.popn});
        insert(mode, Troops,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = sect.troops});
        insert(mode, mPopn,
               {.x = sect.x,
                .y = sect.y,
                .des = sect.condition,
                .value = maxsupport(race, sect, planet.compatibility(race),
                                    planet.conditions[TOXIC])});
      }
    }
  }

  sprintf(buf, "\nAnalysis of /%s/%s:\n", stars[Starnum].name,
          stars[Starnum].pnames[Planetnum]);
  notify(Playernum, Governor, buf);
  sprintf(buf, "%s %d", (mode == Mode::top_five ? "Highest" : "Lowest"), CARE);
  switch (sector_type) {
    case -1:
      sprintf(buf, "%s of all", buf);
      break;
    case SectorType::SEC_SEA:
      sprintf(buf, "%s Ocean", buf);
      break;
    case SectorType::SEC_LAND:
      sprintf(buf, "%s Land", buf);
      break;
    case SectorType::SEC_MOUNT:
      sprintf(buf, "%s Mountain", buf);
      break;
    case SectorType::SEC_GAS:
      sprintf(buf, "%s Gas", buf);
      break;
    case SectorType::SEC_ICE:
      sprintf(buf, "%s Ice", buf);
      break;
    case SectorType::SEC_FOREST:
      sprintf(buf, "%s Forest", buf);
      break;
    case SectorType::SEC_DESERT:
      sprintf(buf, "%s Desert", buf);
      break;
    case SectorType::SEC_PLATED:
      sprintf(buf, "%s Plated", buf);
      break;
    case SectorType::SEC_WASTED:
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

  PrintTop(g, Troops, "Troops");
  PrintTop(g, Res, "Res");
  PrintTop(g, Eff, "Eff");
  PrintTop(g, Frt, "Frt");
  PrintTop(g, Mob, "Mob");
  PrintTop(g, Popn, "Popn");
  PrintTop(g, mPopn, "^Popn");

  g.out << "\n";
  sprintf(buf, "%2s %3s %7s %6s %5s %5s %5s %2s", "Pl", "sec", "popn", "troops",
          "a.eff", "a.mob", "res", "x");
  notify(Playernum, Governor, buf);

  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    sprintf(buf, "%4c", SectTypes[i]);
    notify(Playernum, Governor, buf);
  }
  notify(Playernum, Governor,
         "\n----------------------------------------------"
         "---------------------------------\n");
  for (player_t p = 0; p <= Num_races; p++)
    if (PlayTSect[p] != 0) {
      sprintf(buf, "%2d %3d %7d %6d %5.1lf %5.1lf %5d %2d", p, PlayTSect[p],
              PlayPopn[p], PlayTroops[p], (double)PlayEff[p] / PlayTSect[p],
              (double)PlayMob[p] / PlayTSect[p], PlayRes[p], PlayCrys[p]);
      notify(Playernum, Governor, buf);
      for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
        sprintf(buf, "%4d", PlaySect[p][i]);
        notify(Playernum, Governor, buf);
      }
      g.out << "\n";
    }
  notify(Playernum, Governor,
         "------------------------------------------------"
         "-------------------------------\n");
  sprintf(buf, "%2s %3d %7d %6d %5.1lf %5.1lf %5d %2d", "Tl", TotalSect,
          TotalPopn, TotalTroops, (double)TotalEff / TotalSect,
          (double)TotalMob / TotalSect, TotalRes, TotalCrys);
  notify(Playernum, Governor, buf);
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    sprintf(buf, "%4d", Sect[i]);
    notify(Playernum, Governor, buf);
  }
  g.out << "\n";
}
