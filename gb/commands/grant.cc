// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file grant.cc

#include "gb/commands/grant.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void grant(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // int APcount = 0; TODO(jeffbailey);
  racetype *Race;
  governor_t gov;
  shipnum_t nextshipno;
  shipnum_t shipno;
  Ship *ship;

  Race = races[Playernum - 1];
  if (argv.size() < 3) {
    g.out << "Syntax: grant <governor> star\n";
    g.out << "        grant <governor> ship <shiplist>\n";
    g.out << "        grant <governor> money <amount>\n";
    return;
  }
  if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "Bad governor number.\n";
    return;
  }
  if (!Race->governor[gov].active) {
    g.out << "That governor is not active.\n";
    return;
  }
  if (argv[2] == "star") {
    int snum;
    if (g.level != ScopeLevel::LEVEL_STAR) {
      g.out << "Please cs to the star system first.\n";
      return;
    }
    snum = g.snum;
    Stars[snum]->governor[Playernum - 1] = gov;
    sprintf(buf, "\"%s\" has granted you control of the /%s star system.\n",
            Race->governor[Governor].name, Stars[snum]->name);
    warn(Playernum, gov, buf);
    putstar(Stars[snum], snum);
  } else if (argv[2] == "ship") {
    nextshipno = start_shiplist(g, argv[3]);
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (in_list(Playernum, argv[3], *ship, &nextshipno) &&
          authorized(Governor, *ship)) {
        ship->governor = gov;
        sprintf(buf, "\"%s\" granted you %s at %s\n",
                Race->governor[Governor].name, ship_to_string(*ship).c_str(),
                prin_ship_orbits(ship));
        warn(Playernum, gov, buf);
        putship(ship);
        sprintf(buf, "%s granted to \"%s\"\n", ship_to_string(*ship).c_str(),
                Race->governor[gov].name);
        notify(Playernum, Governor, buf);
        free(ship);
      } else
        free(ship);
  } else if (argv[2] == "money") {
    long amount;
    if (argv.size() < 4) {
      g.out << "Indicate the amount of money.\n";
      return;
    }
    amount = std::stoi(argv[3]);
    if (amount < 0 && Governor) {
      g.out << "Only leaders may make take away money.\n";
      return;
    }
    if (amount > Race->governor[Governor].money)
      amount = Race->governor[Governor].money;
    else if (-amount > Race->governor[gov].money)
      amount = -Race->governor[gov].money;
    if (amount >= 0)
      sprintf(buf, "%ld money granted to \"%s\".\n", amount,
              Race->governor[gov].name);
    else
      sprintf(buf, "%ld money deducted from \"%s\".\n", -amount,
              Race->governor[gov].name);
    notify(Playernum, Governor, buf);
    if (amount >= 0)
      sprintf(buf, "\"%s\" granted you %ld money.\n",
              Race->governor[Governor].name, amount);
    else
      sprintf(buf, "\"%s\" docked you %ld money.\n",
              Race->governor[Governor].name, -amount);
    warn(Playernum, gov, buf);
    Race->governor[Governor].money -= amount;
    Race->governor[gov].money += amount;
    putrace(Race);
    return;
  } else
    g.out << "You can't grant that.\n";
}