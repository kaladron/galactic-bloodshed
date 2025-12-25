// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace {

struct returns {
  int invest = 0;
  double gain = 0;
  double max_gain = 0;
};

void tech_report_star(GameObj& g, const Star& star, starnum_t snum,
                      tabulate::Table& table, returns& totals) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (!isset(star.explored(), Playernum) ||
      (Governor && star.governor(Playernum - 1) != Governor)) {
    return;
  };

  for (planetnum_t i = 0; i < star.numplanets(); i++) {
    const auto* pl = g.entity_manager.peek_planet(snum, i);
    if (!pl || !pl->info(Playernum - 1).explored ||
        !pl->info(Playernum - 1).numsectsowned) {
      continue;
    }

    std::string location =
        std::format("{}/{}{}", star.get_name(), star.get_planet_name(i),
                    (pl->info(Playernum - 1).autorep ? "*" : ""));

    auto gain = tech_prod(pl->info(Playernum - 1).tech_invest,
                          pl->info(Playernum - 1).popn);
    auto max_gain = tech_prod(pl->info(Playernum - 1).prod_res,
                              pl->info(Playernum - 1).popn);

    table.add_row({location, std::format("{}", pl->info(Playernum - 1).popn),
                   std::format("{}", pl->info(Playernum - 1).tech_invest),
                   std::format("{:.3f}", gain), std::format("{:.3f}", max_gain)});

    totals.invest += pl->info(Playernum - 1).tech_invest;
    totals.gain += gain;
    totals.max_gain += max_gain;
  }
}
}  // namespace

namespace GB::commands {
void tech_status(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;

  const auto& sdata = *g.entity_manager.peek_universe();

  g.out << "             ========== Technology Report ==========\n\n";

  // Create table
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure columns
  table.column(0).format().width(16);  // Planet
  table.column(1).format().width(10).font_align(tabulate::FontAlign::right);  // popn
  table.column(2).format().width(10).font_align(tabulate::FontAlign::right);  // invest
  table.column(3).format().width(8).font_align(tabulate::FontAlign::right);   // gain
  table.column(4).format().width(8).font_align(tabulate::FontAlign::right);   // ^gain

  // Add header
  table.add_row({"Planet", "popn", "invest", "gain", "^gain"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  returns totals{};
  if (argv.size() == 1) {
    for (auto star_handle : StarList(g.entity_manager)) {
      const auto& star = *star_handle;
      tech_report_star(g, star, star.star_id(), table, totals);
    }
  } else { /* Several arguments */
    for (int k = 1; k < argv.size(); k++) {
      Place where{g, argv[k]};
      if (where.err || where.level == ScopeLevel::LEVEL_UNIV ||
          where.level == ScopeLevel::LEVEL_SHIP) {
        g.out << std::format("Bad location `{}`.\n", argv[k]);
        continue;
      } /* ok, a proper location */
      starnum_t star = where.snum;
      const auto* star_ptr = g.entity_manager.peek_star(star);
      if (!star_ptr) continue;
      Star star_wrapper(*star_ptr);
      tech_report_star(g, star_wrapper, star, table, totals);
    }
  }

  g.out << table << "\n";

  const auto* power_ptr = g.entity_manager.peek_power(Playernum);
  if (!power_ptr) {
    g.out << "       Total Popn:  unknown\n";
  } else {
    g.out << std::format("       Total Popn:  {:7}\n", power_ptr->popn);
  }
  g.out << std::format("Tech: {:31}{:8.3f}{:8.3f}\n", totals.invest,
                       totals.gain, totals.max_gain);
}
}  // namespace GB::commands