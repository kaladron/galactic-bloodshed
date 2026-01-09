// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void treasury(const command_t&, GameObj& g) {
  governor_t Governor = g.governor();

  const auto& gov = g.race->governor[Governor.value];
  money_t total_income = gov.income + gov.profit_market;
  money_t total_costs = gov.maintain + gov.cost_tech + gov.cost_market;

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure columns: Income label, Income value, spacer, Costs label, Costs
  // value
  table.column(0).format().width(12);
  table.column(1).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(2).format().width(4);  // spacer
  table.column(3).format().width(12);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::right);

  // Add header row
  table.add_row({"Income", std::format("{}", total_income), "", "Costs",
                 std::format("{}", total_costs)});
  table[0].format().font_style({tabulate::FontStyle::bold});

  // Add detail rows
  table.add_row({"  Market:", std::format("{}", gov.profit_market), "",
                 "  Market:", std::format("{}", gov.cost_market)});
  table.add_row({"  Taxes:", std::format("{}", gov.income), "",
                 "  Tech:", std::format("{}", gov.cost_tech)});
  table.add_row({"", "", "", "  Maint:", std::format("{}", gov.maintain)});

  g.out << table << "\n";
  g.out << std::format("You have: {}\n", gov.money);
}
}  // namespace GB::commands
