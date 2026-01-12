// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace {
void colonies_at_star(GameObj& g, const Race& race, const starnum_t star) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  const auto& star_ref = *g.entity_manager.peek_star(star);
  if (!isset(star_ref.explored(), Playernum)) return;

  for (auto i = 0; i < star_ref.numplanets(); i++) {
    const auto& pl = *g.entity_manager.peek_planet(star, i);

    if (!pl.info(Playernum).explored || !pl.info(Playernum).numsectsowned ||
        (Governor != 0 && star_ref.governor(Playernum) != Governor)) {
      continue;
    }

    auto formatted = std::format(
        " {:c} {:4.4}/{:<4.4}{:c}{:4d}{:3d}{:5d}{:8d}{:3d}{:6d}{:5d}{:6d} "
        "{:3d}/{:<3d}{:3.0f}/{:<3d}{:3d}/{:<3d}",
        Psymbol[pl.type()], star_ref.get_name(), star_ref.get_planet_name(i),
        (pl.info(Playernum).autorep ? '*' : ' '), star_ref.governor(Playernum),
        pl.info(Playernum).numsectsowned, pl.info(Playernum).tech_invest,
        pl.info(Playernum).popn, pl.info(Playernum).crystals,
        pl.info(Playernum).resource, pl.info(Playernum).destruct,
        pl.info(Playernum).fuel, pl.info(Playernum).tax,
        pl.info(Playernum).newtax, pl.compatibility(race), pl.conditions(TOXIC),
        pl.info(Playernum).comread, pl.info(Playernum).mob_set);
    g.out << formatted;
    for (player_t j{1}; j <= g.entity_manager.num_races(); ++j)
      if ((j != Playernum) && (pl.info(j).numsectsowned > 0)) {
        auto race_str = std::format(" {}", j);
        g.out << race_str;
      }
    g.out << "\n";
  }
}
}  // namespace

namespace GB::commands {
void colonies(const command_t& argv, GameObj& g) {
  g.out << "          ========== Colonization Report ==========\n";
  g.out << "  Planet     gov sec tech    popn  x   res  "
           "des  fuel  tax  cmpt/tox mob  Aliens\n";

  if (argv.size() < 2)
    for (auto star_handle : StarList(g.entity_manager)) {
      const auto& star = *star_handle;
      colonies_at_star(g, *g.race, star.star_id());
    }
  else
    for (int i = 1; i < argv.size(); i++) {
      Place where{g, argv[i]};
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        auto error_msg = std::format("Bad location `{}'.\n", argv[i]);
        g.out << error_msg;
        continue;
      } /* ok, a proper location */
      colonies_at_star(g, *g.race, where.snum);
    }
  g.out << "\n";
}
}  // namespace GB::commands
