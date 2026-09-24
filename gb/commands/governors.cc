// SPDX-License-Identifier: Apache-2.0

/// \file governors.cc
/// \brief Governor management commands.

module;

import gb.entities;
import gb.services;
import notification;
import session;
import std;
import tabulate;

module commands;

namespace {
void do_revoke(Race& race, const governor_t src_gov, const governor_t tgt_gov,
               EntityManager& entity_manager) {
  std::string outmsg =
      std::format("*** Transferring [{0},{1}]'s ownings to [{2},{3}] ***\n\n",
                  race.Playernum, src_gov, race.Playernum, tgt_gov);
  push_telegram(entity_manager, race.Playernum, Race::leader_id, outmsg);

  /*  First do stars....  */
  for (auto star_handle : StarList(entity_manager)) {
    auto& star = *star_handle;
    if (star.governor(race.Playernum) == src_gov) {
      star.governor(race.Playernum) = tgt_gov;
      outmsg = std::format("Changed juridiction of /{0}...\n", star.get_name());
      push_telegram(entity_manager, race.Playernum, Race::leader_id, outmsg);
    }
  }

  /*  Now do ships....  */
  auto num_ships = entity_manager.num_ships();
  for (shipnum_t i = 1; i <= num_ships; i++) {
    entity_manager.mutate_ship(i, [&](Ship& ship) {
      if (ship.alive() && (ship.owner() == race.Playernum) &&
          (ship.governor() == src_gov)) {
        ship.governor() = tgt_gov;
        outmsg = std::format("Changed ownership of {0}{1}...\n",
                             ship.type_letter(), i);
        push_telegram(entity_manager, race.Playernum, Race::leader_id, outmsg);
      }
    });
  }

  /*  Transfer commodity market lots and bids....  */
  for (auto commod_handle : CommodList(entity_manager)) {
    auto& c = *commod_handle;
    if (c.owner == race.Playernum && c.governor == src_gov) {
      c.governor = tgt_gov;
    }
    if (c.bidder == race.Playernum && c.bidder_gov == src_gov) {
      c.bidder_gov = tgt_gov;
    }
  }

  /*  And money too....  */
  const money_t transferred = race.revoke_governor(src_gov, tgt_gov);
  outmsg = std::format("Transferring {0} money...\n", transferred);
  push_telegram(entity_manager, race.Playernum, Race::leader_id, outmsg);
  outmsg =
      std::format("\n*** Governor [{0},{1}]'s powers have been REVOKED ***\n",
                  race.Playernum, src_gov);
  push_telegram(entity_manager, race.Playernum, Race::leader_id, outmsg);
}
}  // namespace

namespace GB::commands {

bool governors(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (argv[0] == "governors") {
    if (argv.size() >= 4 && argv[2] == "password") {
      int raw_gov = 0;
      try {
        raw_gov = std::stoi(argv[1]);
      } catch (...) {
        g.out << "No such governor.\n";
        return false;
      }
      if (raw_gov < 1) {
        g.out << "No such governor.\n";
        return false;
      }
      governor_t gov{static_cast<governor_t::value_type>(raw_gov)};
      if (!g.is_leader() && Governor != gov) {
        g.out << "You can only change your own password.\n";
        return false;
      }
      bool success = false;
      g.entity_manager.mutate_race(Playernum, [&](Race& race) {
        if (race.Guest) {
          g.out << "Guest races cannot change passwords.\n";
          return;
        }
        if (!race.has_governor(gov)) {
          g.out << "That governor is inactive.\n";
          return;
        }
        race.governor(gov).password = argv[3];
        g.out << "Password changed.\n";
        success = true;
      });
      return success;
    }

    // Default: render governors table
    g.entity_manager.with_race(Playernum, [&](const Race& race) {
      tabulate::Table table;
      table.format().hide_border().column_separator("  ");

      // Configure columns - password at end, only shown to leader
      table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
      table.column(1).format().width(15);
      table.column(2).format().width(10).font_align(tabulate::FontAlign::right);
      table.column(3).format().width(24);
      if (g.is_leader()) {
        table.column(4).format().width(10);
        table.add_row({"#", "Name", "Money", "Last Login", "Password"});
      } else {
        table.add_row({"#", "Name", "Money", "Last Login"});
      }
      table[0].format().font_style({tabulate::FontStyle::bold});

      for (auto [i, g_entry] : race.active_governors()) {
        std::string login_time = std::ctime(&g_entry.login);
        // Remove trailing newline from ctime
        if (!login_time.empty() && login_time.back() == '\n') {
          login_time.pop_back();
        }

        std::vector<std::string> row = {
            std::format("{}", i.value), std::string(g_entry.name),
            std::format("{}", g_entry.money), login_time};
        if (g.is_leader()) {
          row.emplace_back(g_entry.password);
        }
        table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
      }
      g.out << table << "\n";
    });
    return true;
  }

  if (argv[0] == "appoint") {
    if (!g.is_leader()) {
      g.out << "Only the race leader may appoint governors.\n";
      return false;
    }
    if (argv.size() < 2) {
      g.out << "Syntax: appoint [<gov>] <password>\n";
      return false;
    }
    if (argv.size() == 2) {
      g.entity_manager.mutate_race(Playernum, [&](Race& race) {
        const governor_t gov = race.appoint_governor({.password = argv[1]});
        g.out << std::format("Governor {} activated.\n", gov);
      });
      return true;
    }
    int raw_gov = 0;
    try {
      raw_gov = std::stoi(argv[1]);
    } catch (...) {
      g.out << "No such governor.\n";
      return false;
    }
    if (raw_gov < 1) {
      g.out << "No such governor.\n";
      return false;
    }
    governor_t gov{static_cast<governor_t::value_type>(raw_gov)};

    /* Syntax: 'appoint <gov> <password>' */
    bool success = false;
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      if (race.has_governor(gov)) {
        g.out << "That governor is already appointed.\n";
        return;
      }
      race.appoint_governor(gov, {.password = argv[2]});
      g.out << "Governor activated.\n";
      success = true;
    });
    return success;
  }

  if (argv[0] == "revoke") {
    if (!g.is_leader()) {
      g.out << "Only the race leader may revoke governors.\n";
      return false;
    }
    if (argv.size() < 3) {
      g.out << "Syntax: revoke <gov> <password> [<target>]\n";
      return false;
    }
    int raw_gov = 0;
    try {
      raw_gov = std::stoi(argv[1]);
    } catch (...) {
      g.out << "No such governor.\n";
      return false;
    }
    if (raw_gov < 1) {
      g.out << "No such governor.\n";
      return false;
    }
    governor_t gov{static_cast<governor_t::value_type>(raw_gov)};

    if (Race::is_leader(gov)) {
      g.out << "You can't revoke your leadership!\n";
      return false;
    }
    governor_t j = Race::leader_id;
    if (argv.size() >= 4) {
      try {
        int raw_j = std::stoi(argv[3]);
        if (raw_j < 1) {
          g.out << "You can't give stuff to that governor!\n";
          return false;
        }
        j = governor_t{static_cast<governor_t::value_type>(raw_j)};
      } catch (...) {
        g.out << "You can't give stuff to that governor!\n";
        return false;
      }
    }
    bool success = false;
    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      if (!race.has_governor(gov)) {
        g.out << "That governor is not active.\n";
        return;
      }
      if (race.governor(gov).password != argv[2]) {
        g.out << "Incorrect password.\n";
        return;
      }
      if (!race.has_governor(j) || j == gov) {
        g.out << "Bad target governor.\n";
        return;
      }
      do_revoke(race, gov, j, g.entity_manager);
      g.out << "Done.\n";
      success = true;
    });
    return success;
  }

  g.out << "Bad option.\n";
  return false;
}

namespace {
constexpr std::array<std::string_view, 2> kGovernorsAliases{"appoint",
                                                            "revoke"};
}

const CommandDescriptor governors_cmd{
    .name = "governors",
    .aliases = kGovernorsAliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "governors | appoint <gov> <password> | revoke <gov> <password> "
              "[<target>]",
    .description = "List, appoint, or revoke race governors",
    .handler = &governors,
};

}  // namespace GB::commands
