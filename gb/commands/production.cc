// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std;
import tabulate;

module commands;

namespace {
void production_at_star(GameObj& g, starnum_t star, tabulate::Table& table) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  const auto& star_ref = *g.entity_manager.peek_star(star);
  if (!isset(star_ref.explored(), Playernum)) return;

  for (auto i = 0; i < star_ref.numplanets(); i++) {
    const auto& pl = *g.entity_manager.peek_planet(star, i);

    if (pl.info(Playernum - 1).explored &&
        pl.info(Playernum - 1).numsectsowned &&
        (!Governor || star_ref.governor(Playernum - 1) == Governor)) {
      const auto star4 = std::string(star_ref.get_name()).substr(0, 4);
      const auto planet4 =
          std::string(star_ref.get_planet_name(i)).substr(0, 4);

      std::string autorep = pl.info(Playernum - 1).autorep ? "*" : " ";

      table.add_row({std::string(1, Psymbol[pl.type()]),
                     std::format("{}/{}", star4, planet4),
                     autorep,
                     std::format("{}", star_ref.governor(Playernum - 1)),
                     std::format("{:.4f}", pl.info(Playernum - 1).prod_tech),
                     std::format("{}", pl.total_resources()),
                     std::format("{}", pl.info(Playernum - 1).prod_crystals),
                     std::format("{}", pl.info(Playernum - 1).prod_res),
                     std::format("{}", pl.info(Playernum - 1).prod_dest),
                     std::format("{}", pl.info(Playernum - 1).prod_fuel),
                     std::format("{}", pl.info(Playernum - 1).prod_money),
                     std::format("{}", pl.info(Playernum - 1).tox_thresh),
                     std::format("{:.2f}", pl.info(Playernum - 1).est_production)});
    }
  }
}
}  // namespace

namespace GB::commands {
void production(const command_t& argv, GameObj& g) {
  g.out << "          ============ Production Report ==========\n";

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure columns
  table.column(0).format().width(1);   // Planet type symbol
  table.column(1).format().width(9);   // Star/Planet
  table.column(2).format().width(1);   // Autorep flag
  table.column(3).format().width(3).font_align(tabulate::FontAlign::right);   // Gov
  table.column(4).format().width(8).font_align(tabulate::FontAlign::right);   // Tech
  table.column(5).format().width(8).font_align(tabulate::FontAlign::right);   // Deposit
  table.column(6).format().width(3).font_align(tabulate::FontAlign::right);   // Crystals
  table.column(7).format().width(6).font_align(tabulate::FontAlign::right);   // Res
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);   // Dest
  table.column(9).format().width(6).font_align(tabulate::FontAlign::right);   // Fuel
  table.column(10).format().width(6).font_align(tabulate::FontAlign::right);  // Tax
  table.column(11).format().width(3).font_align(tabulate::FontAlign::right);  // Tox
  table.column(12).format().width(8).font_align(tabulate::FontAlign::right);  // Est prod

  // Add header
  table.add_row({"", "Planet", "", "gov", "tech", "deposit", "x", "res", "des", "fuel", "tax", "tox", "est prod"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  if (argv.size() < 2)
    for (auto star_handle : StarList(g.entity_manager)) {
      const auto& star = *star_handle;
      production_at_star(g, star.star_id(), table);
    }
  else
    for (int i = 1; i < argv.size(); i++) {
      Place where{g, argv[i]};
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        g.out << std::format("Bad location `{}`.\n", argv[i]);
        continue;
      } /* ok, a proper location */
      production_at_star(g, where.snum, table);
    }

  g.out << table << "\n";
}
}  // namespace GB::commands
