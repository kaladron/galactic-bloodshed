// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void block(const command_t& argv, GameObj& g) {
  player_t p;

  const auto* race = g.race;

  if (argv.size() == 3 && argv[1] == "player") {
    if (!(p = get_player(g.entity_manager, argv[2]))) {
      g.out << "No such player.\n";
      return;
    }
    const auto* r = g.entity_manager.peek_race(p);
    if (!r) {
      g.out << "Race not found.\n";
      return;
    }
    // Flag for finding a block
    bool found_any = false;
    g.out << std::format("Race #{} [{}] is a member of ", p, r->name);
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (isset(block_i->pledge, p) && isset(block_i->invite, p)) {
        g.out << std::format("{}{}", found_any ? ", " : " ", i);
        found_any = true;
      }
    }
    if (!found_any)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    found_any = false;
    g.out << std::format("Race #{} [{}] has been invited to join ", p, r->name);
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (!isset(block_i->pledge, p) && isset(block_i->invite, p)) {
        g.out << std::format("{}{}", found_any ? ", " : " ", i);
        found_any = true;
      }
    }
    if (!found_any)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    found_any = false;
    g.out << std::format("Race #{} [{}] has pledged ", p, r->name);
    for (int i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i) continue;
      if (isset(block_i->pledge, p) && !isset(block_i->invite, p)) {
        g.out << std::format("{}{}", found_any ? ", " : " ", i);
        found_any = true;
      }
    }
    if (!found_any)
      g.out << "no blocks\n";
    else
      g.out << "\n";
  } else if (argv.size() > 1) {
    if (!(p = get_player(g.entity_manager, argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
    /* list the players who are in this alliance block */
    const auto* block_p = g.entity_manager.peek_block(p);
    if (!block_p) {
      g.out << "Block not found.\n";
      return;
    }
    std::uint64_t allied_members = (block_p->invite & block_p->pledge);
    g.out << std::format("         ========== {} Power Report ==========\n",
                         block_p->name);
    g.out << std::format("                 {:<64.64}\n", block_p->motto);

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
    table.column(1).format().width(20);
    table.column(2).format().width(6).font_align(tabulate::FontAlign::right);
    table.column(3).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(6).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(7).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(9).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(10).format().width(4).font_align(tabulate::FontAlign::right);

    table.add_row({"#", "Name", "troops", "pop", "money", "ship", "plan", "res",
                   "fuel", "dest", "know"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (player_t i = 1; i <= g.entity_manager.num_races(); i++) {
      if (!isset(allied_members, i)) continue;
      const auto* r = g.entity_manager.peek_race(i);
      if (!r || r->dissolved) continue;
      const auto* power_ptr = g.entity_manager.peek_power(r->Playernum);
      if (!power_ptr) continue;

      table.add_row({std::format("{}", r->Playernum), std::string(r->name),
                     estimate(power_ptr->troops, *race, r->Playernum),
                     estimate(power_ptr->popn, *race, r->Playernum),
                     estimate(power_ptr->money, *race, r->Playernum),
                     estimate(power_ptr->ships_owned, *race, r->Playernum),
                     estimate(power_ptr->planets_owned, *race, r->Playernum),
                     estimate(power_ptr->resource, *race, r->Playernum),
                     estimate(power_ptr->fuel, *race, r->Playernum),
                     estimate(power_ptr->destruct, *race, r->Playernum),
                     std::format("{}%", race->translate[r->Playernum - 1])});
    }

    g.out << table << "\n";
  } else {
    /* list power report for all the alliance blocks (as of the last update) */
    std::string time_str = std::asctime(std::localtime(&Power_blocks.time));
    // Remove trailing newline from asctime
    if (!time_str.empty() && time_str.back() == '\n') {
      time_str.pop_back();
    }
    g.out << std::format(
        "         ========== Alliance Blocks as of {} ==========\n", time_str);

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
    table.column(1).format().width(19);
    table.column(2).format().width(4).font_align(tabulate::FontAlign::right);
    table.column(3).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(6).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(7).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(9).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(10).format().width(5).font_align(tabulate::FontAlign::right);
    table.column(11).format().width(4).font_align(tabulate::FontAlign::right);

    table.add_row({"#", "Name", "memb", "money", "popn", "ship", "sys", "res",
                   "fuel", "dest", "VPs", "know"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (auto i = 1; i <= g.entity_manager.num_races(); i++) {
      const auto* block_i = g.entity_manager.peek_block(i);
      if (!block_i || Power_blocks.members[i - 1] == 0) continue;

      table.add_row({std::format("{}", i), std::string(block_i->name),
                     std::format("{}", Power_blocks.members[i - 1]),
                     estimate(Power_blocks.money[i - 1], *race, i),
                     estimate(Power_blocks.popn[i - 1], *race, i),
                     estimate(Power_blocks.ships_owned[i - 1], *race, i),
                     estimate(Power_blocks.systems_owned[i - 1], *race, i),
                     estimate(Power_blocks.resource[i - 1], *race, i),
                     estimate(Power_blocks.fuel[i - 1], *race, i),
                     estimate(Power_blocks.destruct[i - 1], *race, i),
                     estimate(Power_blocks.VPs[i - 1], *race, i),
                     std::format("{}%", race->translate[i - 1])});
    }

    g.out << table << "\n";
  }
}
}  // namespace GB::commands
