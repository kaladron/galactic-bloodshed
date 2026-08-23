// SPDX-License-Identifier: Apache-2.0

/// \file victory.cc
/// \brief Display current victory standings and player rankings.

module;

import gblib;
import scnlib;
import std;
import tabulate;
#undef stdout

module commands;

namespace GB::commands {
bool victory(const command_t& argv, GameObj& g) {
  int count = g.entity_manager.num_races().value;
  if (argv.size() > 1) {
    auto scan_res = scn::scan<int>(argv[1], "{}");
    if (!scan_res || scan_res->value() < 1) {
      g.out << "Invalid count specified.\n";
      return false;
    }
    count = scan_res->value();
  }
  count = std::min(count, static_cast<int>(g.entity_manager.num_races().value));

  auto viclist = create_victory_list(g.entity_manager);

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

  if (g.god()) {
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
  tabulate::Table::Row_t header = {"No.", "", "Race", "Name"};
  if (g.god()) {
    header.insert(header.end(),
                  {"Score", "Tech", "IQ", "Password", "Gov Pass"});
  }
  table.add_row(header);
  table[0].format().font_style({tabulate::FontStyle::bold});

  // Add data rows
  for (int rank = 0; auto& vic : viclist) {
    rank++;
    if (rank > count) break;

    // Build base row
    tabulate::Table::Row_t row = {
        std::format("{}", rank), std::format("{}", vic.thing ? 'M' : ' '),
        std::format("[{}]", vic.racenum), std::format("{:.15}", vic.name)};

    // Add god-only columns
    if (g.god()) {
      const auto* race = g.entity_manager.peek_race(vic.racenum);
      if (!race) continue;

      row.insert(row.end(),
                 {std::format("{}", vic.rawscore),
                  std::format("{:.2f}", vic.tech), std::format("{}", vic.iq),
                  std::format("{}", race->password),
                  std::format("{}", race->governor[0].password)});
    }

    table.add_row(row);
  }

  g.out << table << "\n";
  return true;
}

const CommandDescriptor victory_cmd{
    .name = "victory",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "victory [<count>]",
    .description = "Display current victory standings and player rankings",
    .handler = &victory,
};

}  // namespace GB::commands
