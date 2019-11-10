// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  toggle.c -- toggles some options */

import gblib;
import std;

#include "gb/commands/toggle.h"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/vars.h"

namespace {
void tog(GameObj &g, char *op, const char *name) {
  *op = !(*op);
  g.out << fmt::format("{0} is now {1}\n", name, *op ? "on" : "off");
}
}  // namespace

void toggle(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  racetype *Race;

  Race = races[Playernum - 1];

  if (argv.size() > 1) {
    if (argv[1] == "inverse")
      tog(g, &Race->governor[Governor].toggle.inverse, "inverse");
    else if (argv[1] == "double_digits")
      tog(g, &Race->governor[Governor].toggle.double_digits, "double_digits");
    else if (argv[1] == "geography")
      tog(g, &Race->governor[Governor].toggle.geography, "geography");
    else if (argv[1] == "gag")
      tog(g, &Race->governor[Governor].toggle.gag, "gag");
    else if (argv[1] == "autoload")
      tog(g, &Race->governor[Governor].toggle.autoload, "autoload");
    else if (argv[1] == "color")
      tog(g, &Race->governor[Governor].toggle.color, "color");
    else if (argv[1] == "visible")
      tog(g, &Race->governor[Governor].toggle.invisible, "invisible");
    else if (Race->God && argv[1] == "monitor")
      tog(g, &Race->monitor, "monitor");
    else if (argv[1] == "compatibility")
      tog(g, &Race->governor[Governor].toggle.compat, "compatibility");
    else {
      sprintf(buf, "No such option '%s'\n", argv[1].c_str());
      notify(Playernum, Governor, buf);
      return;
    }
    putrace(Race);
  } else {
    sprintf(buf, "gag is %s\n",
            Race->governor[Governor].toggle.gag ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "inverse is %s\n",
            Race->governor[Governor].toggle.inverse ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "double_digits is %s\n",
            Race->governor[Governor].toggle.double_digits ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "geography is %s\n",
            Race->governor[Governor].toggle.geography ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "autoload is %s\n",
            Race->governor[Governor].toggle.autoload ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "color is %s\n",
            Race->governor[Governor].toggle.color ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(buf, "compatibility is %s\n",
            Race->governor[Governor].toggle.compat ? "ON" : "OFF");
    notify(Playernum, Governor, buf);
    sprintf(
        buf, "%s\n",
        Race->governor[Governor].toggle.invisible ? "INVISIBLE" : "VISIBLE");
    notify(Playernum, Governor, buf);
    sprintf(buf, "highlight player %d\n",
            Race->governor[Governor].toggle.highlight);
    notify(Playernum, Governor, buf);
    if (Race->God) {
      sprintf(buf, "monitor is %s\n", Race->monitor ? "ON" : "OFF");
      notify(Playernum, Governor, buf);
    }
  }
}
