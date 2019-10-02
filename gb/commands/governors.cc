// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file governors.cc

import gblib;

#include "gb/commands/governors.h"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

import std;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

namespace {
void do_revoke(racetype *Race, const governor_t src_gov,
               const governor_t tgt_gov) {
  std::string outmsg =
      fmt::format("*** Transferring [{0},{1}]'s ownings to [{2},{3}] ***\n\n",
                  Race->Playernum, src_gov, Race->Playernum, tgt_gov);
  notify(Race->Playernum, (governor_t)0, outmsg);

  /*  First do stars....  */

  for (starnum_t i = 0; i < Sdata.numstars; i++)
    if (Stars[i]->governor[Race->Playernum - 1] == src_gov) {
      Stars[i]->governor[Race->Playernum - 1] = tgt_gov;
      outmsg = fmt::format("Changed juridiction of /{0}...\n", Stars[i]->name);
      notify(Race->Playernum, 0, outmsg);
      putstar(Stars[i], i);
    }

  /*  Now do ships....  */
  Num_ships = Numships();
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    auto ship = getship(i);
    if (!ship) continue;
    if (ship->alive && (ship->owner == Race->Playernum) &&
        (ship->governor == src_gov)) {
      ship->governor = tgt_gov;
      outmsg = fmt::format("Changed ownership of {0}{1}...\n",
                           Shipltrs[ship->type], i);
      notify(Race->Playernum, 0, outmsg);
      putship(&*ship);
    }
  }

  /*  And money too....  */

  outmsg =
      fmt::format("Transferring {0} money...\n", Race->governor[src_gov].money);
  notify(Race->Playernum, 0, outmsg);
  Race->governor[tgt_gov].money =
      Race->governor[tgt_gov].money + Race->governor[src_gov].money;
  Race->governor[src_gov].money = 0;

  /* And last but not least, flag the governor as inactive.... */

  Race->governor[src_gov].active = 0;
  strcpy(Race->governor[src_gov].password, "");
  strcpy(Race->governor[src_gov].name, "");
  outmsg =
      fmt::format("\n*** Governor [{0},{1}]'s powers have been REVOKED ***\n",
                  Race->Playernum, src_gov);
  notify(Race->Playernum, 0, outmsg);

  // TODO(jeffbailey): Use C++17 Filesystem stuff when available
  std::string rm_telegram_file =
      fmt::format("rm {0}.{1}.{2}", TELEGRAMFL, Race->Playernum, src_gov);
  if (system(rm_telegram_file.c_str()) <
      0) { /*  Remove the telegram file too....  */
    perror("gaaaaaaaah");
    exit(-1);
  }
}
}  // namespace

void governors(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  racetype *Race;
  governor_t gov;

  Race = races[Playernum - 1];
  if (Governor ||
      argv.size() < 3) { /* the only thing governors can do with this */
    for (governor_t i = 0; i <= MAXGOVERNORS; i++) {
      if (Governor)
        sprintf(buf, "%d %-15.15s %8s %10ld %s", i, Race->governor[i].name,
                Race->governor[i].active ? "ACTIVE" : "INACTIVE",
                Race->governor[i].money, ctime(&Race->governor[i].login));
      else
        sprintf(buf, "%d %-15.15s %-10.10s %8s %10ld %s", i,
                Race->governor[i].name, Race->governor[i].password,
                Race->governor[i].active ? "ACTIVE" : "INACTIVE",
                Race->governor[i].money, ctime(&Race->governor[i].login));
      notify(Playernum, Governor, buf);
    }
  } else if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "No such governor.\n";
    return;
  } else if (argv[0] == "appoint") {
    /* Syntax: 'appoint <gov> <password>' */
    if (Race->governor[gov].active) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    Race->governor[gov].active = 1;
    Race->governor[gov].homelevel = Race->governor[gov].deflevel =
        Race->governor[0].deflevel;
    Race->governor[gov].homesystem = Race->governor[gov].defsystem =
        Race->governor[0].defsystem;
    Race->governor[gov].homeplanetnum = Race->governor[gov].defplanetnum =
        Race->governor[0].defplanetnum;
    Race->governor[gov].money = 0;
    Race->governor[gov].toggle.highlight = Playernum;
    Race->governor[gov].toggle.inverse = 1;
    strncpy(Race->governor[gov].password, argv[2].c_str(), RNAMESIZE - 1);
    putrace(Race);
    g.out << "Governor activated.\n";
    return;
  } else if (argv[0] == "revoke") {
    governor_t j;
    if (!gov) {
      g.out << "You can't revoke your leadership!\n";
      return;
    }
    if (!Race->governor[gov].active) {
      g.out << "That governor is not active.\n";
      return;
    }
    if (argv.size() < 4)
      j = 0;
    else
      j = std::stoul(argv[3]); /* who gets this governors stuff */
    if (j > MAXGOVERNORS) {
      g.out << "You can't give stuff to that governor!\n";
      return;
    }
    if (!strcmp(Race->governor[gov].password, argv[2].c_str())) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!Race->governor[j].active || j == gov) {
      g.out << "Bad target governor.\n";
      return;
    }
    do_revoke(Race, gov, j); /* give stuff from gov to j */
    putrace(Race);
    g.out << "Done.\n";
    return;
  } else if (argv[2] == "password") {
    if (Race->Guest) {
      g.out << "Guest races cannot change passwords.\n";
      return;
    }
    if (argv.size() < 4) {
      g.out << "You must give a password.\n";
      return;
    }
    if (!Race->governor[gov].active) {
      g.out << "That governor is inactive.\n";
      return;
    }
    strncpy(Race->governor[gov].password, argv[3].c_str(), RNAMESIZE - 1);
    putrace(Race);
    g.out << "Password changed.\n";
    return;
  } else
    g.out << "Bad option.\n";
}
