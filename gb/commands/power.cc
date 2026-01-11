// SPDX-License-Identifier: Apache-2.0

// \file power.c display power report

module;

import gblib;
import std;
import tabulate;

module commands;

namespace {
void add_power_row(tabulate::Table& table, EntityManager& em, const Race& race,
                   const Race& r, player_t i, int rank) {
  std::string rank_col = (rank != 0) ? std::format("{}", rank) : "";

  std::string alliance_them = isset(race.allied, i)  ? "+"
                              : isset(race.atwar, i) ? "-"
                                                     : " ";
  std::string alliance_us = isset(r.allied, race.Playernum)  ? "+"
                            : isset(r.atwar, race.Playernum) ? "-"
                                                             : " ";

  const auto* power_ptr = em.peek_power(i.value);
  if (!power_ptr) return;

  std::string know_col;
  if (race.God) {
    const auto* universe = em.peek_universe();
    know_col = std::format("{}", universe->VN_hitlist[i.value - 1]);
  } else {
    know_col = std::format("{}%", race.translate[i.value - 1]);
  }

  table.add_row(
      {rank_col, std::format("[{:2d}]", i), alliance_them + alliance_us,
       std::string(r.name), estimate(r.victory_score, race, i),
       estimate(power_ptr->troops, race, i), estimate(power_ptr->popn, race, i),
       estimate(power_ptr->money, race, i),
       estimate(power_ptr->ships_owned, race, i),
       estimate(power_ptr->planets_owned, race, i),
       estimate(power_ptr->resource, race, i),
       estimate(power_ptr->fuel, race, i),
       estimate(power_ptr->destruct, race, i), estimate(r.morale, race, i),
       know_col});
}
}  // namespace

namespace GB::commands {
void power(const command_t& argv, GameObj& g) {
  std::optional<player_t> target_player;

  if (argv.size() >= 2) {
    player_t p = get_player(g.entity_manager, argv[1]);
    if (p == 0) {
      g.out << "No such player,\n";
      return;
    }
    target_player = p;
  }

  const auto* race = g.race;

  g.out << "         ========== Galactic Bloodshed Power Report ==========\n";

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure columns
  table.column(0).format().width(4).font_align(
      tabulate::FontAlign::right);     // rank
  table.column(1).format().width(4);   // #
  table.column(2).format().width(2);   // alliance
  table.column(3).format().width(15);  // name
  table.column(4).format().width(5).font_align(
      tabulate::FontAlign::right);  // VP
  table.column(5).format().width(5).font_align(
      tabulate::FontAlign::right);  // mil
  table.column(6).format().width(5).font_align(
      tabulate::FontAlign::right);  // civ
  table.column(7).format().width(5).font_align(
      tabulate::FontAlign::right);  // cash
  table.column(8).format().width(5).font_align(
      tabulate::FontAlign::right);  // ship
  table.column(9).format().width(3).font_align(
      tabulate::FontAlign::right);  // pl
  table.column(10).format().width(5).font_align(
      tabulate::FontAlign::right);  // res
  table.column(11).format().width(5).font_align(
      tabulate::FontAlign::right);  // fuel
  table.column(12).format().width(5).font_align(
      tabulate::FontAlign::right);  // dest
  table.column(13).format().width(5).font_align(
      tabulate::FontAlign::right);  // morl
  table.column(14).format().width(5).font_align(
      tabulate::FontAlign::right);  // know/VNs

  // Add header
  std::string rank_header = (argv.size() < 2) ? "rank" : "";
  std::string know_header = race->God ? "VNs" : "know";
  table.add_row({rank_header, "#", "", "Name", "VP", "mil", "civ", "cash",
                 "ship", "pl", "res", "fuel", "dest", "morl", know_header});
  table[0].format().font_style({tabulate::FontStyle::bold});

  if (argv.size() < 2) {
    auto vicvec = create_victory_list(g.entity_manager);
    int rank = 0;
    for (const auto& vic : vicvec) {
      rank++;
      player_t p = vic.racenum;
      const auto* r = g.entity_manager.peek_race(p);
      if (!r) continue;
      if (!r->dissolved && race->translate[p.value - 1] >= 10) {
        add_power_row(table, g.entity_manager, *race, *r, p, rank);
      }
    }
  } else {
    const auto* r = g.entity_manager.peek_race(target_player.value());
    if (!r) {
      g.out << "Race not found.\n";
      return;
    }
    add_power_row(table, g.entity_manager, *race, *r, target_player.value(), 0);
  }

  g.out << table << "\n";
}
}  // namespace GB::commands
