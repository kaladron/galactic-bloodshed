// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file governors.cc

module;

import gblib;
import session;
import notification;
import std;
import tabulate;

module commands;

namespace {
void do_revoke(Race& race, const governor_t src_gov, const governor_t tgt_gov,
               EntityManager& entity_manager) {
  std::string outmsg =
      std::format("*** Transferring [{0},{1}]'s ownings to [{2},{3}] ***\n\n",
                  race.Playernum, src_gov, race.Playernum, tgt_gov);
  push_telegram(entity_manager, race.Playernum, (governor_t)0, outmsg);

  /*  First do stars....  */

  for (auto star_handle : StarList(entity_manager)) {
    auto& star = *star_handle;
    if (star.governor(race.Playernum) == src_gov) {
      star.governor(race.Playernum) = tgt_gov;
      outmsg = std::format("Changed juridiction of /{0}...\n", star.get_name());
      push_telegram(entity_manager, race.Playernum, 0, outmsg);
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
      push_telegram(entity_manager, race.Playernum, 0, outmsg);
    }
  }

  /*  And money too....  */

  outmsg = std::format("Transferring {0} money...\n",
                       race.governor[src_gov.value].money);
  push_telegram(entity_manager, race.Playernum, 0, outmsg);
  race.governor[tgt_gov.value].money =
      race.governor[tgt_gov.value].money + race.governor[src_gov.value].money;
  race.governor[src_gov.value].money = 0;

  /* And last but not least, flag the governor as inactive.... */

  race.governor[src_gov.value].active = false;
  race.governor[src_gov.value].password = "";
  race.governor[src_gov.value].name = "";
  outmsg =
      std::format("\n*** Governor [{0},{1}]'s powers have been REVOKED ***\n",
                  race.Playernum, src_gov);
  push_telegram(entity_manager, race.Playernum, 0, outmsg);
}
}  // namespace

namespace GB::commands {
void governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  governor_t gov;

  auto race = g.entity_manager.get_race(Playernum);
  if (Governor != 0 ||
      argv.size() < 3) { /* the only thing governors can do with this */
    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    // Configure columns - password at end, only shown to governor 0
    table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
    table.column(1).format().width(15);
    table.column(2).format().width(8);
    table.column(3).format().width(10).font_align(tabulate::FontAlign::right);
    table.column(4).format().width(24);
    if (Governor == 0) {
      table.column(5).format().width(10);
      table.add_row({"#", "Name", "Status", "Money", "Last Login", "Password"});
    } else {
      table.add_row({"#", "Name", "Status", "Money", "Last Login"});
    }
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (auto [i, gov] : race->all_governors()) {
      std::string status = gov.active ? "ACTIVE" : "INACTIVE";
      std::string login_time = std::ctime(&gov.login);
      // Remove trailing newline from ctime
      if (!login_time.empty() && login_time.back() == '\n') {
        login_time.pop_back();
      }

      std::vector<std::string> row = {std::format("{}", i.value),
                                      std::string(gov.name), status,
                                      std::format("{}", gov.money), login_time};
      if (Governor == 0) {
        row.emplace_back(gov.password);
      }
      table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
    }
    g.out << table << "\n";
  } else if ((gov = std::stoi(argv[1])) > MAXGOVERNORS) {
    g.out << "No such governor.\n";
    return;
  } else if (argv[0] == "appoint") {
    /* Syntax: 'appoint <gov> <password>' */
    if (race->governor[gov.value].active) {
      g.out << "That governor is already appointed.\n";
      return;
    }
    race->governor[gov.value].active = true;
    race->governor[gov.value].homelevel = race->governor[gov.value].deflevel =
        race->governor[0].deflevel;
    race->governor[gov.value].homesystem = race->governor[gov.value].defsystem =
        race->governor[0].defsystem;
    race->governor[gov.value].homeplanetnum =
        race->governor[gov.value].defplanetnum = race->governor[0].defplanetnum;
    race->governor[gov.value].money = 0;
    race->governor[gov.value].toggle.highlight = Playernum;
    race->governor[gov.value].toggle.inverse = 1;
    race->governor[gov.value].password = argv[2];
    // Auto-saves via EntityHandle RAII
    g.out << "Governor activated.\n";
    return;
  } else if (argv[0] == "revoke") {
    governor_t j;
    if (gov == 0) {
      g.out << "You can't revoke your leadership!\n";
      return;
    }
    if (!race->governor[gov.value].active) {
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
    if (race->governor[gov.value].password != argv[2]) {
      g.out << "Incorrect password.\n";
      return;
    }
    if (!race->governor[j.value].active || j == gov) {
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
    if (!race->governor[gov.value].active) {
      g.out << "That governor is inactive.\n";
      return;
    }
    race->governor[gov.value].password = argv[3];
    g.out << "Password changed.\n";
    return;
  } else
    g.out << "Bad option.\n";
}
}  // namespace GB::commands
