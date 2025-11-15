// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void victory(const command_t& argv, GameObj& g) {
  int count = (argv.size() > 1) ? std::stoi(argv[1]) : Num_races;
  count = std::min<player_t>(count, Num_races);

  auto viclist = create_victory_list();

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(3).font_align(
      tabulate::FontAlign::right);  // No.
  table.column(1).format().width(1).font_align(
      tabulate::FontAlign::center);  // M flag
  table.column(2).format().width(4).font_align(
      tabulate::FontAlign::center);    // [Race]
  table.column(3).format().width(15);  // Name

  if (g.god) {
    table.column(4).format().width(6).font_align(
        tabulate::FontAlign::right);  // Score
    table.column(5).format().width(6).font_align(
        tabulate::FontAlign::right);  // Tech
    table.column(6).format().width(3).font_align(
        tabulate::FontAlign::right);     // IQ
    table.column(7).format().width(10);  // Password
    table.column(8).format().width(10);  // Gov Pass
  }

  // Add header
  g.out << "----==== PLAYER RANKINGS ====----\n";

  // Add header row
  if (g.god) {
    table.add_row({"No.", "", "Race", "Name", "Score", "Tech", "IQ", "Password",
                   "Gov Pass"});
  } else {
    table.add_row({"No.", "", "Race", "Name"});
  }
  table[0].format().font_style({tabulate::FontStyle::bold});

  // Add data rows
  for (int i = 0; auto& vic : viclist) {
    i++;

    if (g.god) {
      const auto* race = g.entity_manager.peek_race(vic.racenum);
      if (!race) continue;

      table.add_row(
          {std::format("{}", i), std::format("{}", vic.Thing ? 'M' : ' '),
           std::format("[{}]", vic.racenum), std::format("{:.15}", vic.name),
           std::format("{}", vic.rawscore), std::format("{:.2f}", vic.tech),
           std::format("{}", vic.IQ), std::format("{}", race->password),
           std::format("{}", race->governor[0].password)});
    } else {
      table.add_row(
          {std::format("{}", i), std::format("{}", vic.Thing ? 'M' : ' '),
           std::format("[{}]", vic.racenum), std::format("{:.15}", vic.name)});
    }
  }

  g.out << table << "\n";
}
}  // namespace GB::commands
