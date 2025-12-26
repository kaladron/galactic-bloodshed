// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void star_locations(const command_t& argv, GameObj& g) {
  // Optional argument filters to only show stars within this distance
  int max_dist = (argv.size() > 1) ? std::stoi(argv[1]) : 999999;

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.column(0).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(20);
  table.column(2).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(7).font_align(tabulate::FontAlign::right);

  table.add_row({"#", "Name", "X", "Y", "Dist"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (auto star_handle : StarList(g.entity_manager)) {
    const auto& star = *star_handle;
    auto dist =
        std::sqrt(Distsq(star.xpos(), star.ypos(), g.lastx[1], g.lasty[1]));
    if (std::floor(dist) <= max_dist) {
      table.add_row(
          {std::format("{}", star.star_id()), std::string(star.get_name()),
           std::format("{:.0f}", star.xpos()),
           std::format("{:.0f}", star.ypos()), std::format("{:.0f}", dist)});
    }
  }

  if (table.size() > 1) {
    g.out << table << "\n";
  } else {
    g.out << "No stars found within specified distance.\n";
  }
}
}  // namespace GB::commands
