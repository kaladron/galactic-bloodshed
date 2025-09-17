// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file governors.cc

module;

import gblib;
import std.compat;
#include "gb/files.h"

module commands;

namespace {
void do_revoke(Race race, const governor_t src_gov, const governor_t tgt_gov) {
  std::string outmsg =
      std::format("*** Transferring [{0},{1}]'s ownings to [{2},{3}] ***\n\n",
                  race.Playernum, src_gov, race.Playernum, tgt_gov);
  notify(race.Playernum, (governor_t)0, outmsg);

  /*  First do stars....  */

  for (starnum_t i = 0; i < Sdata.numstars; i++)
    if (stars[i].governor(race.Playernum - 1) == src_gov) {
      stars[i].governor(race.Playernum - 1) = tgt_gov;
      outmsg =
          std::format("Changed juridiction of /{0}...\n", stars[i].get_name());
      notify(race.Playernum, 0, outmsg);
      putstar(stars[i], i);
    }

  /*  Now do ships....  */
  Num_ships = Numships();
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    auto ship = getship(i);
    if (!ship) continue;
    if (ship->alive && (ship->owner == race.Playernum) &&
        (ship->governor == src_gov)) {
      ship->governor = tgt_gov;
      outmsg = std::format("Changed ownership of {0}{1}...\n",
                           Shipltrs[ship->type], i);
      notify(race.Playernum, 0, outmsg);
      putship(*ship);
    }
  }

  /*  And money too....  */

  outmsg =
      std::format("Transferring {0} money...\n", race.governor[src_gov].money);
  notify(race.Playernum, 0, outmsg);
  race.governor[tgt_gov].money =
      race.governor[tgt_gov].money + race.governor[src_gov].money;
  race.governor[src_gov].money = 0;

  /* And last but not least, flag the governor as inactive.... */

  race.governor[src_gov].active = false;
  race.governor[src_gov].password = "";
  race.governor[src_gov].name = "";
  outmsg =
      std::format("\n*** Governor [{0},{1}]'s powers have been REVOKED ***\n",
                  race.Playernum, src_gov);
  notify(race.Playernum, 0, outmsg);

  // TODO(jeffbailey): Use C++17 Filesystem stuff when available
  std::string rm_telegram_file =
      std::format("rm {0}.{1}.{2}", TELEGRAMFL, race.Playernum, src_gov);
  if (system(rm_telegram_file.c_str()) <
      0) { /*  Remove the telegram file too....  */
    perror("gaaaaaaaah");
    exit(-1);
  }
}
}  // namespace

namespace GB::commands {
void governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  governor_t gov;

  auto& race = races[Playernum - 1];
  if (Governor ||
      argv.size() < 3) { /* the only thing governors can do with this */
    for (governor_t i = 0; i <= MAXGOVERNORS; i++) {
      auto line =
          Governor
              ? std::format("{0:>2} {1:<15.15} {2:>8} {3:>10} {4}", i,
                            race.governor[i].name,
                            race.governor[i].active ? "ACTIVE" : "INACTIVE",
                            race.governor[i].money,
                            ctime(&race.governor[i].login))
              : std::format("{0:>2} {1:<15.15} {2:<10.10} {3:>8} {4:>10} {5}",
                            i, race.governor[i].name, race.governor[i].password,
                            race.governor[i].active ? "ACTIVE" : "INACTIVE",
                            race.governor[i].money,
                            ctime(&race.governor[i].login));
      notify(Playernum, Governor, line);
    }
  } else if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "No such governor.\n";
    return;
  } else if (argv[0] == "appoint") {
    /* Syntax: 'appoint <gov> <password>' */
    if (race.governor[gov].active) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    race.governor[gov].active = true;
    race.governor[gov].homelevel = race.governor[gov].deflevel =
        race.governor[0].deflevel;
    race.governor[gov].homesystem = race.governor[gov].defsystem =
        race.governor[0].defsystem;
    race.governor[gov].homeplanetnum = race.governor[gov].defplanetnum =
        race.governor[0].defplanetnum;
    race.governor[gov].money = 0;
    race.governor[gov].toggle.highlight = Playernum;
    race.governor[gov].toggle.inverse = 1;
    race.governor[gov].password = argv[2];
    putrace(race);
    g.out << "Governor activated.\n";
    return;
  } else if (argv[0] == "revoke") {
    governor_t j;
    if (!gov) {
      g.out << "You can't revoke your leadership!\n";
      return;
    }
    if (!race.governor[gov].active) {
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
    if (race.governor[gov].password != argv[2]) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!race.governor[j].active || j == gov) {
      g.out << "Bad target governor.\n";
      return;
    }
    do_revoke(race, gov, j); /* give stuff from gov to j */
    putrace(race);
    g.out << "Done.\n";
    return;
  } else if (argv[2] == "password") {
    if (race.Guest) {
      g.out << "Guest races cannot change passwords.\n";
      return;
    }
    if (argv.size() < 4) {
      g.out << "You must give a password.\n";
      return;
    }
    if (!race.governor[gov].active) {
      g.out << "That governor is inactive.\n";
      return;
    }
    race.governor[gov].password = argv[3];
    putrace(race);
    g.out << "Password changed.\n";
    return;
  } else
    g.out << "Bad option.\n";
}
}  // namespace GB::commands
