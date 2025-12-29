// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file governors.cc

module;

import gblib;
import std;
import tabulate;
#include "gb/files.h"

module commands;

namespace {
void do_revoke(Race& race, const governor_t src_gov, const governor_t tgt_gov,
               EntityManager& entity_manager) {
  std::string outmsg =
      std::format("*** Transferring [{0},{1}]'s ownings to [{2},{3}] ***\n\n",
                  race.Playernum, src_gov, race.Playernum, tgt_gov);
  notify(race.Playernum, (governor_t)0, outmsg);

  /*  First do stars....  */

  for (auto star_handle : StarList(entity_manager)) {
    auto& star = *star_handle;
    if (star.governor(race.Playernum - 1) == src_gov) {
      star.governor(race.Playernum - 1) = tgt_gov;
      outmsg = std::format("Changed juridiction of /{0}...\n", star.get_name());
      notify(race.Playernum, 0, outmsg);
    }
  }

  /*  Now do ships....  */
  auto num_ships = entity_manager.num_ships();
  for (shipnum_t i = 1; i <= num_ships; i++) {
    auto ship = entity_manager.get_ship(i);
    if (!ship.get()) continue;
    if (ship->alive() && (ship->owner() == race.Playernum) &&
        (ship->governor() == src_gov)) {
      ship->governor() = tgt_gov;
      outmsg = std::format("Changed ownership of {0}{1}...\n",
                           Shipltrs[ship->type()], i);
      notify(race.Playernum, 0, outmsg);
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
  if (std::system(rm_telegram_file.c_str()) <
      0) { /*  Remove the telegram file too....  */
    std::perror("gaaaaaaaah");
    std::exit(-1);
  }
}
}  // namespace

namespace GB::commands {
void governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  governor_t gov;

  auto race = g.entity_manager.get_race(Playernum);
  if (Governor ||
      argv.size() < 3) { /* the only thing governors can do with this */
    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    // Configure columns - password at end, only shown to governor 0
    table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
    table.column(1).format().width(15);
    table.column(2).format().width(8);
    table.column(3).format().width(10).font_align(tabulate::FontAlign::right);
    table.column(4).format().width(24);
    if (!Governor) {
      table.column(5).format().width(10);
      table.add_row({"#", "Name", "Status", "Money", "Last Login", "Password"});
    } else {
      table.add_row({"#", "Name", "Status", "Money", "Last Login"});
    }
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (governor_t i = 0; i <= MAXGOVERNORS; i++) {
      std::string status = race->governor[i].active ? "ACTIVE" : "INACTIVE";
      std::string login_time = std::ctime(&race->governor[i].login);
      // Remove trailing newline from ctime
      if (!login_time.empty() && login_time.back() == '\n') {
        login_time.pop_back();
      }

      std::vector<std::string> row = {
          std::format("{}", i), std::string(race->governor[i].name), status,
          std::format("{}", race->governor[i].money), login_time};
      if (!Governor) {
        row.emplace_back(race->governor[i].password);
      }
      table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
    }
    g.out << table << "\n";
  } else if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "No such governor.\n";
    return;
  } else if (argv[0] == "appoint") {
    /* Syntax: 'appoint <gov> <password>' */
    if (race->governor[gov].active) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    race->governor[gov].active = true;
    race->governor[gov].homelevel = race->governor[gov].deflevel =
        race->governor[0].deflevel;
    race->governor[gov].homesystem = race->governor[gov].defsystem =
        race->governor[0].defsystem;
    race->governor[gov].homeplanetnum = race->governor[gov].defplanetnum =
        race->governor[0].defplanetnum;
    race->governor[gov].money = 0;
    race->governor[gov].toggle.highlight = Playernum;
    race->governor[gov].toggle.inverse = 1;
    race->governor[gov].password = argv[2];
    // Auto-saves via EntityHandle RAII
    g.out << "Governor activated.\n";
    return;
  } else if (argv[0] == "revoke") {
    governor_t j;
    if (!gov) {
      g.out << "You can't revoke your leadership!\n";
      return;
    }
    if (!race->governor[gov].active) {
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
    if (race->governor[gov].password != argv[2]) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!race->governor[j].active || j == gov) {
      g.out << "Bad target governor.\n";
      return;
    }
    do_revoke(*race, gov, j, g.entity_manager); /* give stuff from gov to j */
    g.out << "Done.\n";
    return;
  } else if (argv[2] == "password") {
    if (race->Guest) {
      g.out << "Guest races cannot change passwords.\n";
      return;
    }
    if (argv.size() < 4) {
      g.out << "You must give a password.\n";
      return;
    }
    if (!race->governor[gov].active) {
      g.out << "That governor is inactive.\n";
      return;
    }
    race->governor[gov].password = argv[3];
    g.out << "Password changed.\n";
    return;
  } else
    g.out << "Bad option.\n";
}
}  // namespace GB::commands
