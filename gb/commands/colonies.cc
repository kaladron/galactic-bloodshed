// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace {
void colonies_at_star(GameObj& g, const Race& race, const starnum_t star) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  const auto& star_ref = *g.entity_manager.peek_star(star);
  if (!isset(star_ref.explored(), Playernum)) return;

  for (auto i = 0; i < star_ref.numplanets(); i++) {
    const auto& pl = *g.entity_manager.peek_planet(star, i);

    if (!pl.info(Playernum - 1).explored ||
        !pl.info(Playernum - 1).numsectsowned ||
        (Governor && star_ref.governor(Playernum - 1) != Governor)) {
      continue;
    }

    auto formatted = std::format(
        " {:c} {:4.4}/{:<4.4}{:c}{:4d}{:3d}{:5d}{:8d}{:3d}{:6d}{:5d}{:6d} "
        "{:3d}/{:<3d}{:3.0f}/{:<3d}{:3d}/{:<3d}",
        Psymbol[pl.type()], star_ref.get_name(), star_ref.get_planet_name(i),
        (pl.info(Playernum - 1).autorep ? '*' : ' '),
        star_ref.governor(Playernum - 1), pl.info(Playernum - 1).numsectsowned,
        pl.info(Playernum - 1).tech_invest, pl.info(Playernum - 1).popn,
        pl.info(Playernum - 1).crystals, pl.info(Playernum - 1).resource,
        pl.info(Playernum - 1).destruct, pl.info(Playernum - 1).fuel,
        pl.info(Playernum - 1).tax, pl.info(Playernum - 1).newtax,
        pl.compatibility(race), pl.conditions(TOXIC),
        pl.info(Playernum - 1).comread, pl.info(Playernum - 1).mob_set);
    g.out << formatted;
    for (auto j = 1; j <= Num_races; j++)
      if ((j != Playernum) && (pl.info(j - 1).numsectsowned > 0)) {
        auto race_str = std::format(" {}", j);
        g.out << race_str;
      }
    g.out << "\n";
  }
}
}  // namespace

namespace GB::commands {
void colonies(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;

  g.out << "          ========== Colonization Report ==========\n";
  g.out << "  Planet     gov sec tech    popn  x   res  "
           "des  fuel  tax  cmpt/tox mob  Aliens\n";

  auto& race = races[Playernum - 1];
  getsdata(&Sdata);

  if (argv.size() < 2)
    for (starnum_t star = 0; star < Sdata.numstars; star++)
      colonies_at_star(g, race, star);
  else
    for (int i = 1; i < argv.size(); i++) {
      Place where{g, argv[i]};
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        auto error_msg = std::format("Bad location `{}'.\n", argv[i]);
        g.out << error_msg;
        continue;
      } /* ok, a proper location */
      colonies_at_star(g, race, where.snum);
    }
  g.out << "\n";
}
}  // namespace GB::commands
