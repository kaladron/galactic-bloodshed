// SPDX-License-Identifier: Apache-2.0

/// \file tech_status.cc
/// \brief Technology investment and generation report for colonies.

module;

import gblib;
import std;
import tabulate;
#undef stdout

module commands;

namespace {

struct returns {
  int invest = 0;
  double gain = 0;
  double max_gain = 0;
};

void tech_report_star(GameObj& g, const Star& star, starnum_t snum,
                      tabulate::Table& table, returns& totals) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();

  if (!isset(star.explored(), Playernum) ||
      (Governor != 0 && star.governor(Playernum) != Governor)) {
    return;
  };

  for (planetnum_t i = 0; i < star.numplanets(); i++) {
    const auto* pl = g.entity_manager.peek_planet(snum, i);
    if (!pl || !pl->info(Playernum).explored ||
        !pl->info(Playernum).numsectsowned) {
      continue;
    }

    std::string location =
        std::format("{}/{}{}", star.get_name(), star.get_planet_name(i),
                    (pl->info(Playernum).autorep ? "*" : ""));

    auto gain =
        tech_prod(pl->info(Playernum).tech_invest, pl->info(Playernum).popn);
    auto max_gain =
        tech_prod(pl->info(Playernum).prod_res, pl->info(Playernum).popn);

    table.add_row({location, std::format("{}", pl->info(Playernum).popn),
                   std::format("{}", pl->info(Playernum).tech_invest),
                   std::format("{:.3f}", gain),
                   std::format("{:.3f}", max_gain)});

    totals.invest += pl->info(Playernum).tech_invest;
    totals.gain += gain;
    totals.max_gain += max_gain;
  }
}
}  // namespace

namespace GB::commands {
bool tech_status(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();

  g.out << "             ========== Technology Report ==========\n\n";

  // Create table
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure columns
  table.column(0).format().width(16);  // Planet
  table.column(1).format().width(10).font_align(
      tabulate::FontAlign::right);  // popn
  table.column(2).format().width(10).font_align(
      tabulate::FontAlign::right);  // invest
  table.column(3).format().width(8).font_align(
      tabulate::FontAlign::right);  // gain
  table.column(4).format().width(8).font_align(
      tabulate::FontAlign::right);  // ^gain

  // Add header
  table.add_row({"Planet", "popn", "invest", "gain", "^gain"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  returns totals{};
  if (argv.size() == 1) {
    for (const Star& star : StarList::readonly(g.entity_manager)) {
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
      try {
        const auto& star_ref = *g.entity_manager.peek_star(star);
        tech_report_star(g, star_ref, star, table, totals);
      } catch (const EntityNotFoundError&) {
        continue;
      }
    }
  }

  g.out << table << "\n";

  const auto* power_ptr =
      g.entity_manager.peek_power(powernum_t{Playernum.value});
  if (!power_ptr) {
    g.out << "       Total Popn:  unknown\n";
  } else {
    g.out << std::format("       Total Popn:  {:7}\n", power_ptr->popn);
  }
  g.out << std::format("Tech: {:31}{:8.3f}{:8.3f}\n", totals.invest,
                       totals.gain, totals.max_gain);
  return true;
}

const CommandDescriptor status_cmd{
    .name = "status",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "status [<star> ...]",
    .description = "Technology investment and generation report for colonies",
    .handler = &tech_status,
};

}  // namespace GB::commands