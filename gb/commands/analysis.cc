// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"

module commands;

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
              const std::string &name) {
  g.out << std::format("{:>8}:", name);

  for (const auto &as : arr) {
    if (as.value == -1) continue;
    sprintf(buf, "%5ld%c(%2d,%2d)", as.value, Dessymbols[as.des], as.x, as.y);
    notify(g.player, g.governor, buf);
  }
  g.out << "\n";
}

void do_analysis(GameObj &g, int ThisPlayer, Mode mode, int sector_type,
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
  int TotalCrys = 0;
  std::array<int, MAXPLAYERS + 1> PlayCrys;
  int TotalTroops = 0;
  std::array<int, MAXPLAYERS + 1> PlayTroops;
  int TotalPopn = 0;
  std::array<int, MAXPLAYERS + 1> PlayPopn;
  int TotalMob = 0;
  std::array<int, MAXPLAYERS + 1> PlayMob;
  int TotalEff = 0;
  std::array<int, MAXPLAYERS + 1> PlayEff;
  int TotalRes = 0;
  std::array<int, MAXPLAYERS + 1> PlayRes;
  int TotalSect = 0;
  int PlaySect[MAXPLAYERS + 1][SectorType::SEC_WASTED + 1];
  std::array<int, MAXPLAYERS + 1> PlayTSect;
  std::array<int, MAXPLAYERS + 1> WastedSect;
  std::array<int, SectorType::SEC_WASTED + 1> Sect;
  static char SectTypes[] = {CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,
                             CHAR_GAS,    CHAR_ICE,    CHAR_FOREST,
                             CHAR_DESERT, CHAR_PLATED, CHAR_WASTED};

  for (int i = 0; i < CARE; i++)
    Res[i].value = Eff[i].value = Frt[i].value = Mob[i].value =
        Troops[i].value = Popn[i].value = mPopn[i].value = -1;

  PlayTroops.fill(0);
  PlayPopn.fill(0);
  PlayMob.fill(0);
  PlayEff.fill(0);
  PlayCrys.fill(0);
  PlayRes.fill(0);
  PlayTSect.fill(0);
  WastedSect.fill(0);

  for (int p = 0; p <= Num_races; p++) {
    for (int i = 0; i <= SectorType::SEC_WASTED; i++) PlaySect[p][i] = 0;
  }

  Sect.fill(0);

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

  g.out << std::format("\nAnalysis of /{}/{}:\n", stars[Starnum].name,
                       stars[Starnum].pnames[Planetnum]);
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
  g.out << std::format("{:>2} {:>3} {:>7} {:>6} {:>5} {:>5} {:>5} {:>2}", "Pl",
                       "sec", "popn", "troops", "a.eff", "a.mob", "res", "x");

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

}  // namespace

namespace GB::commands {
void analysis(const command_t &argv, GameObj &g) {
  int sector_type = -1; /* -1 does analysis on all types */
  int do_player = -1;
  auto mode = Mode::top_five;

  auto where = Place{g.level, g.snum, g.pnum};

  bool skipped_first = false;

  for (const auto &arg : argv) {
    // Skip the name of the command
    if (!skipped_first) {
      skipped_first = true;
      continue;
    }

    // Top or bottom five
    if (arg == "-") {
      mode = Mode::bottom_five;
      continue;
    }

    // Sector type
    if (arg.length() == 1) {
      switch (arg[0]) {
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
          /*  Must use 'd' to do an analysis on */
          /*  desert sectors to avoid confusion */
        /*  with the '-' for the mode type    */
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
      if (sector_type != -1) {
        continue;
      }
    }

    // Player number
    if (isdigit(arg[0])) {
      do_player = std::stoi(arg);
      if (do_player > Num_races) {
        g.out << "No such player #.\n";
        return;
      }
    }

    // Scope
    Place maybe_where{g, arg};
    if (!maybe_where.err) {
      where = maybe_where;
    }
  }

  if (where.err) {
    g.out << "Invalid scope.\n";
    return;
  }

  switch (where.level) {
    case ScopeLevel::LEVEL_UNIV:
      [[fallthrough]];
    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can only analyze planets.\n";
      break;
    case ScopeLevel::LEVEL_PLAN:
      do_analysis(g, do_player, mode, sector_type, where.snum, where.pnum);
      break;
    case ScopeLevel::LEVEL_STAR:
      for (planetnum_t pnum = 0; pnum < stars[where.snum].numplanets; pnum++)
        do_analysis(g, do_player, mode, sector_type, where.snum, pnum);
      break;
  }
}
}  // namespace GB::commands