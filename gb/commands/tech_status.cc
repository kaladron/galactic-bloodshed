// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace {

struct returns {
  int invest = 0;
  double gain = 0;
  double max_gain = 0;
};

returns tech_report_star(GameObj& g, const Star& star, starnum_t snum) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (!isset(star.explored(), Playernum) ||
      (Governor && star.governor(Playernum - 1) != Governor)) {
    return {};
  };

  returns totals{};
  for (planetnum_t i = 0; i < star.numplanets(); i++) {
    const auto* pl = g.entity_manager.peek_planet(snum, i);
    if (!pl || !pl->info(Playernum - 1).explored ||
        !pl->info(Playernum - 1).numsectsowned) {
      continue;
    }

    std::string location =
        std::format("{}/{}/{}", star.get_name(), star.get_planet_name(i),
                    (pl->info(Playernum - 1).autorep ? "*" : ""));

    auto gain = tech_prod(pl->info(Playernum - 1).tech_invest,
                          pl->info(Playernum - 1).popn);
    auto max_gain = tech_prod(pl->info(Playernum - 1).prod_res,
                              pl->info(Playernum - 1).popn);

    g.out << std::format("{:16.16} {:10} {:10} {:8.3f} {:8.3f}\n", location,
                         pl->info(Playernum - 1).popn,
                         pl->info(Playernum - 1).tech_invest, gain, max_gain);
    totals.invest += pl->info(Playernum - 1).tech_invest;
    totals.gain += gain;
    totals.max_gain += max_gain;
  }
  return totals;
}
}  // namespace

// FIXME(jeffbailey): Use tabulate here.
namespace GB::commands {
void tech_status(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;

  const auto& sdata = *g.entity_manager.peek_universe();

  g.out << std::format(
      "             ========== Technology Report ==========\n\n");

  g.out << std::format(
      "       Planet          popn    invest    gain   ^gain\n");

  returns totals{};
  if (argv.size() == 1) {
    for (starnum_t star = 0; star < sdata.numstars; star++) {
      const auto* star_ptr = g.entity_manager.peek_star(star);
      if (!star_ptr) continue;
      Star star_wrapper(*star_ptr);
      totals = tech_report_star(g, star_wrapper, star);
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
      tech_report_star(g, star_wrapper, star);
    }
  }
  g.out << std::format("       Total Popn:  {:7}\n", Power[Playernum - 1].popn);
  g.out << std::format("Tech: {:31}{:8.3f}{:8.3f}\n", totals.invest,
                       totals.gain, totals.max_gain);
}
}  // namespace GB::commands