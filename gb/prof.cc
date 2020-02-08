// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* prof.c -- print out racial profile */

import gblib;
import std;

#include "gb/prof.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/shootblast.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static int round_perc(int, const Race &, int);
static char *Estimate_f(double, const Race &, int);

void whois(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;

  if (argv.size() <= 1) {
    whois({"whois", std::to_string(Playernum)}, g);
    return;
  }
  auto numraces = Num_races;

  for (size_t i = 1; i <= argv.size() - 1; i++) {
    auto j = std::stoi(argv[i]);
    if (!(j < 1 || j > numraces)) {
      auto &race = races[j - 1];
      if (j == Playernum)
        sprintf(buf, "[%2d, %d] %s \"%s\"\n", j, Governor, race.name,
                race.governor[Governor].name);
      else
        sprintf(buf, "[%2d] %s\n", j, race.name);
    } else {
      sprintf(buf, "Identify: Invalid player number #%d. Try again.\n", j);
    }
    notify(Playernum, Governor, buf);
  }
}

void treasury(const command_t &, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  auto &race = races[Playernum - 1];

  sprintf(
      buf, "Income last update was: %ld\t\tCosts last update was: %ld\n",
      race.governor[Governor].income + race.governor[Governor].profit_market,
      race.governor[Governor].maintain + race.governor[Governor].cost_tech +
          race.governor[Governor].cost_market);
  notify(Playernum, Governor, buf);
  sprintf(buf, "    Market: %5ld\t\t\t     Market: %5ld\n",
          race.governor[Governor].profit_market,
          race.governor[Governor].cost_market);
  notify(Playernum, Governor, buf);
  sprintf(buf, "    Taxes:  %5ld\t\t\t       Tech: %5ld\n",
          race.governor[Governor].income, race.governor[Governor].cost_tech);
  notify(Playernum, Governor, buf);

  sprintf(buf, "\t\t\t\t\t      Maint: %5ld\n",
          race.governor[Governor].maintain);
  notify(Playernum, Governor, buf);
  sprintf(buf, "You have: %ld\n", race.governor[Governor].money);
  notify(Playernum, Governor, buf);
}

void profile(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  int p;

  auto &race = races[Playernum - 1];

  if (argv.size() == 1) {
    sprintf(buf, "--==** Racial profile for %s (player %d) **==--\n", race.name,
            race.Playernum);
    notify(Playernum, Governor, buf);
    if (race.God) {
      sprintf(buf, "*** Diety Status ***\n");
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "Personal: %s\n", race.info);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Default Scope: /%s/%s\n",
            stars[race.governor[Governor].homesystem].name,
            stars[race.governor[Governor].homesystem]
                .pnames[race.governor[Governor].homeplanetnum]);
    notify(Playernum, Governor, buf);
    if (race.Gov_ship == 0)
      sprintf(buf, "NO DESIGNATED CAPITAL!!");
    else
      sprintf(buf, "Designated Capital: #%-8lu", race.Gov_ship);
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\tRanges:     guns:   %5.0f\n",
            gun_range(&race, (Ship *)nullptr, 1));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Morale: %5ld\t\t\t\t\t    space:  %5.0f\n", race.morale,
            tele_range(ShipType::OTYPE_STELE, race.tech));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Updates active: %d\t\t\t\t    ground: %5.0f\n\n", race.turn,
            tele_range(ShipType::OTYPE_GTELE, race.tech));
    notify(Playernum, Governor, buf);
    sprintf(buf, "%s  Planet Conditions\t      Sector Preferences\n",
            race.Metamorph ? "Metamorphic Race\t" : "Normal Race\t\t");
    notify(Playernum, Governor, buf);
    sprintf(buf, "Fert:    %3d%%\t\t  Temp:\t%d\n", race.fertilize,
            Temp(race.conditions[TEMP]));
    notify(Playernum, Governor, buf);
    sprintf(
        buf, "Rate:    %3.1f\t\t  methane  %5d%%\t      %-8.8s %c %3.0f%%\n",
        race.birthrate, race.conditions[METHANE], Desnames[SectorType::SEC_SEA],
        CHAR_SEA, race.likes[SectorType::SEC_SEA] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "Mass:    %4.2f\t\t  oxygen   %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.mass, race.conditions[OXYGEN], Desnames[SectorType::SEC_GAS],
            CHAR_GAS, race.likes[SectorType::SEC_GAS] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Fight:   %d\t\t  helium   %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.fighters, race.conditions[HELIUM],
            Desnames[SectorType::SEC_ICE], CHAR_ICE,
            race.likes[SectorType::SEC_ICE] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "Metab:   %4.2f\t\t  nitrogen %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.metabolism, race.conditions[NITROGEN],
            Desnames[SectorType::SEC_MOUNT], CHAR_MOUNT,
            race.likes[SectorType::SEC_MOUNT] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Sexes:   %1d\t\t  CO2      %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.number_sexes, race.conditions[CO2],
            Desnames[SectorType::SEC_LAND], CHAR_LAND,
            race.likes[SectorType::SEC_LAND] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "Explore: %-3.0f%%\t\t  hydrogen %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.adventurism * 100.0, race.conditions[HYDROGEN],
            Desnames[SectorType::SEC_DESERT], CHAR_DESERT,
            race.likes[SectorType::SEC_DESERT] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Avg Int: %3d\t\t  sulfer   %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.IQ, race.conditions[SULFUR], Desnames[SectorType::SEC_FOREST],
            CHAR_FOREST, race.likes[SectorType::SEC_FOREST] * 100.);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "Tech:    %-6.2f\t\t  other    %5d%%\t      %-8.8s %c %3.0f%%\n",
            race.tech, race.conditions[OTHER], Desnames[SectorType::SEC_PLATED],
            CHAR_PLATED, race.likes[SectorType::SEC_PLATED] * 100.);
    notify(Playernum, Governor, buf);

    g.out << "Discoveries:";
    if (Crystal(race)) g.out << "  Crystals";
    if (Hyper_drive(race)) g.out << "  Hyper-drive";
    if (Laser(race)) g.out << "  Combat Lasers";
    if (Cew(race)) g.out << "  Confined Energy Weapons";
    if (Vn(race)) g.out << "  Von Neumann Machines";
    if (Tractor_beam(race)) g.out << "  Tractor Beam";
    if (Transporter(race)) g.out << "  Transporter";
    if (Avpm(race)) g.out << "  AVPM";
    if (Cloak(race)) g.out << "  Cloaking";
    if (Wormhole(race)) g.out << "  Wormhole";
    g.out << "\n";
  } else {
    if (!(p = get_player(argv[1]))) {
      sprintf(buf, "Player does not exist.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    auto &r = races[p - 1];
    sprintf(buf, "------ Race report on %s (%d) ------\n", r.name, p);
    notify(Playernum, Governor, buf);
    if (race.God) {
      if (r.God) {
        sprintf(buf, "*** Deity Status ***\n");
        notify(Playernum, Governor, buf);
      }
    }
    sprintf(buf, "Personal: %s\n", r.info);
    notify(Playernum, Governor, buf);
    sprintf(buf, "%%Know:  %3d%%\n", race.translate[p - 1]);
    notify(Playernum, Governor, buf);
    if (race.translate[p - 1] > 50) {
      sprintf(buf, "%s\t  Planet Conditions\n",
              r.Metamorph ? "Metamorphic Race" : "Normal Race\t");
      notify(Playernum, Governor, buf);
      sprintf(buf, "Fert:    %s", Estimate_i((int)(r.fertilize), race, p));
      notify(Playernum, Governor, buf);
      sprintf(buf, "\t\t  Temp:\t%s\n",
              Estimate_i((int)(r.conditions[TEMP]), race, p));
      notify(Playernum, Governor, buf);
      sprintf(buf, "Rate:    %s%%", Estimate_f(r.birthrate * 100.0, race, p));
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "Unknown Race\t\t  Planet Conditions\n");
      notify(Playernum, Governor, buf);
      sprintf(buf, "Fert:    %s", Estimate_i((int)(r.fertilize), race, p));
      notify(Playernum, Governor, buf);
      sprintf(buf, "\t\t  Temp:\t%s\n",
              Estimate_i((int)(r.conditions[TEMP]), race, p));
      notify(Playernum, Governor, buf);
      sprintf(buf, "Rate:    %s", Estimate_f(r.birthrate, race, p));
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "\t\t  methane  %4s%%\t\tRanges:\n",
            Estimate_i((int)(r.conditions[METHANE]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Mass:    %s", Estimate_f(r.mass, race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  oxygen   %4s%%",
            Estimate_i((int)(r.conditions[OXYGEN]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  guns:   %6s\n",
            Estimate_f(gun_range(&r, (Ship *)nullptr, 1), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Fight:   %s", Estimate_i((int)(r.fighters), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  helium   %4s%%",
            Estimate_i((int)(r.conditions[HELIUM]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  space:  %6s\n",
            Estimate_f(tele_range(ShipType::OTYPE_STELE, r.tech), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Metab:   %s", Estimate_f(r.metabolism, race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  nitrogen %4s%%",
            Estimate_i((int)(r.conditions[NITROGEN]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  ground: %6s\n",
            Estimate_f(tele_range(ShipType::OTYPE_GTELE, r.tech), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Sexes:   %s", Estimate_i((int)(r.number_sexes), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  CO2      %4s%%\n",
            Estimate_i((int)(r.conditions[CO2]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Explore: %s%%", Estimate_f(r.adventurism * 100.0, race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  hydrogen %4s%%\n",
            Estimate_i((int)(r.conditions[HYDROGEN]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Avg Int: %s", Estimate_i((int)(r.IQ), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  sulfer   %4s%%\n",
            Estimate_i((int)(r.conditions[SULFUR]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Tech:    %s", Estimate_f(r.tech, race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\t  other    %4s%%",
            Estimate_i((int)(r.conditions[OTHER]), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "\t\tMorale:   %6s\n", Estimate_i((int)(r.morale), race, p));
    notify(Playernum, Governor, buf);
    sprintf(buf, "Sector type preference : %s\n",
            race.translate[p - 1] > 80 ? Desnames[r.likesbest] : " ? ");
    notify(Playernum, Governor, buf);
  }
}

static char est_buf[20];

static char *Estimate_f(double data, const Race &r, int p) {
  int est;

  sprintf(est_buf, "?");

  if (r.translate[p - 1] > 10) {
    est = round_perc((int)data, r, p);
    if (est < 1000)
      sprintf(est_buf, "%d", est);
    else if (est < 10000)
      sprintf(est_buf, "%.1fK", (double)est / 1000.);
    else if (est < 1000000)
      sprintf(est_buf, "%.0fK", (double)est / 1000.);
    else
      sprintf(est_buf, "%.1fM", (double)est / 1000000.);
  }
  return est_buf;
}

char *Estimate_i(int data, const Race &r, unsigned int p) {
  int est;

  sprintf(est_buf, "?");

  if (r.translate[p - 1] > 10) {
    est = round_perc((int)data, r, p);
    if ((int)abs(est) < 1000)
      sprintf(est_buf, "%d", est);
    else if ((int)abs(est) < 10000)
      sprintf(est_buf, "%.1fK", (double)est / 1000.);
    else if ((int)abs(est) < 1000000)
      sprintf(est_buf, "%.0fK", (double)est / 1000.);
    else
      sprintf(est_buf, "%.1fM", (double)est / 1000000.);
  }
  return est_buf;
}

static int round_perc(const int data, const Race &r, int p) {
  int k = 101 - MIN(r.translate[p - 1], 100);
  return ((data / k) * k);
}
