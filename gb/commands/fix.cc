// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file fix.cc

import gblib;
import std.compat;

#include "gb/commands/fix.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

/** Deity fix-it utilities */
void fix(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (!g.god) {
    notify(Playernum, Governor,
           "This command is only available to the deity.\n");
    return;
  }

  if (argv[1] == "planet") {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "Change scope to the planet first.\n";
      return;
    }
    auto p = getplanet(g.snum, g.pnum);
    if (argv[2] == "Maxx") {
      if (argv.size() > 3) p.Maxx = std::stoi(argv[3]);
      sprintf(buf, "Maxx = %d\n", p.Maxx);
    } else if (argv[2] == "Maxy") {
      if (argv.size() > 3) p.Maxy = std::stoi(argv[3]);
      sprintf(buf, "Maxy = %d\n", p.Maxy);
    } else if (argv[2] == "xpos") {
      if (argv.size() > 3) p.xpos = (double)std::stoi(argv[3]);
      sprintf(buf, "xpos = %f\n", p.xpos);
    } else if (argv[2] == "ypos") {
      if (argv.size() > 3) p.ypos = (double)std::stoi(argv[3]);
      sprintf(buf, "ypos = %f\n", p.ypos);
    } else if (argv[2] == "ships") {
      if (argv.size() > 3) p.ships = std::stoi(argv[3]);
      sprintf(buf, "ships = %ld\n", p.ships);
    } else if (argv[2] == "rtemp") {
      if (argv.size() > 3) p.conditions[RTEMP] = std::stoi(argv[3]);
      sprintf(buf, "RTEMP = %d\n", p.conditions[RTEMP]);
    } else if (argv[2] == "temperature") {
      if (argv.size() > 3) p.conditions[TEMP] = std::stoi(argv[3]);
      sprintf(buf, "TEMP = %d\n", p.conditions[TEMP]);
    } else if (argv[2] == "methane") {
      if (argv.size() > 3) p.conditions[METHANE] = std::stoi(argv[3]);
      sprintf(buf, "METHANE = %d\n", p.conditions[METHANE]);
    } else if (argv[2] == "oxygen") {
      if (argv.size() > 3) p.conditions[OXYGEN] = std::stoi(argv[3]);
      sprintf(buf, "OXYGEN = %d\n", p.conditions[OXYGEN]);
    } else if (argv[2] == "co2") {
      if (argv.size() > 3) p.conditions[CO2] = std::stoi(argv[3]);
      sprintf(buf, "CO2 = %d\n", p.conditions[CO2]);
    } else if (argv[2] == "hydrogen") {
      if (argv.size() > 3) p.conditions[HYDROGEN] = std::stoi(argv[3]);
      sprintf(buf, "HYDROGEN = %d\n", p.conditions[HYDROGEN]);
    } else if (argv[2] == "nitrogen") {
      if (argv.size() > 3) p.conditions[NITROGEN] = std::stoi(argv[3]);
      sprintf(buf, "NITROGEN = %d\n", p.conditions[NITROGEN]);
    } else if (argv[2] == "sulfur") {
      if (argv.size() > 3) p.conditions[SULFUR] = std::stoi(argv[3]);
      sprintf(buf, "SULFUR = %d\n", p.conditions[SULFUR]);
    } else if (argv[2] == "helium") {
      if (argv.size() > 3) p.conditions[HELIUM] = std::stoi(argv[3]);
      sprintf(buf, "HELIUM = %d\n", p.conditions[HELIUM]);
    } else if (argv[2] == "other") {
      if (argv.size() > 3) p.conditions[OTHER] = std::stoi(argv[3]);
      sprintf(buf, "OTHER = %d\n", p.conditions[OTHER]);
    } else if (argv[2] == "toxic") {
      if (argv.size() > 3) p.conditions[TOXIC] = std::stoi(argv[3]);
      sprintf(buf, "TOXIC = %d\n", p.conditions[TOXIC]);
    } else {
      g.out << "No such option for 'fix planet'.\n";
      return;
    }
    notify(Playernum, Governor, buf);
    if (argv.size() > 3) putplanet(p, stars[g.snum], g.pnum);
    return;
  }
  if (argv[1] == "ship") {
    if (g.level != ScopeLevel::LEVEL_SHIP) {
      notify(Playernum, Governor,
             "Change scope to the ship you wish to fix.\n");
      return;
    }
    auto s = getship(g.shipno);
    if (argv[2] == "fuel") {
      if (argv.size() > 3) s->fuel = (double)std::stoi(argv[3]);
      sprintf(buf, "fuel = %f\n", s->fuel);
    } else if (argv[2] == "max_fuel") {
      if (argv.size() > 3) s->max_fuel = std::stoi(argv[3]);
      sprintf(buf, "fuel = %d\n", s->max_fuel);
    } else if (argv[2] == "destruct") {
      if (argv.size() > 3) s->destruct = std::stoi(argv[3]);
      sprintf(buf, "destruct = %d\n", s->destruct);
    } else if (argv[2] == "resource") {
      if (argv.size() > 3) s->resource = std::stoi(argv[3]);
      sprintf(buf, "resource = %lu\n", s->resource);
    } else if (argv[2] == "damage") {
      if (argv.size() > 3) s->damage = std::stoi(argv[3]);
      sprintf(buf, "damage = %d\n", s->damage);
    } else if (argv[2] == "alive") {
      s->alive = 1;
      s->damage = 0;
      sprintf(buf, "%s resurrected\n", ship_to_string(*s).c_str());
    } else if (argv[2] == "dead") {
      s->alive = 0;
      s->damage = 100;
      sprintf(buf, "%s destroyed\n", ship_to_string(*s).c_str());
    } else {
      g.out << "No such option for 'fix ship'.\n";
      return;
    }
    notify(Playernum, Governor, buf);
    putship(&*s);
    return;
  }
  g.out << "Fix what?\n";
}
