// SPDX-License-Identifier: Apache-2.0

/// \file explore.cc
/// \brief Global census and exploration survey of known stars and worlds.

module;

import gb.entities;
import gb.services;
import std;
import tabulate;
#undef stdout

module commands;

namespace GB::commands {
bool explore(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  int starq = -1;

  if (argv.size() == 2) {
    Place where{g, argv[1]};
    if (where.err) {
      g.out << "explore: bad scope.\n";
      return false;
    }
    if (where.level == ScopeLevel::LEVEL_SHIP ||
        where.level == ScopeLevel::LEVEL_UNIV) {
      g.out << std::format("Bad scope '{}'\n", argv[1]);
      return false;
    }
    starq = static_cast<int>(where.snum.value);
  }

  const auto& sdata = *g.entity_manager.peek_universe();
  g.out << "         ========== Exploration Report ==========\n";
  g.out << std::format(" Global action points : [{:2}]\n",
                       sdata.AP[Playernum.value - 1]);

  for (const Star& star_ref : StarList::readonly(g.entity_manager)) {
    if ((starq == -1) || (starq == star_ref.star_id())) {
      if (star_ref.is_explored_by(Playernum)) {
        // Output star header
        if (g.race->tech >= TECH_SEE_STABILITY) {
          g.out << std::format("\n{} ({:2})[{:2}]\n", star_ref.get_name(),
                               star_ref.stability(), star_ref.AP(Playernum));
        } else {
          g.out << std::format("\n{} (/?/?)[{:2}]\n", star_ref.get_name(),
                               star_ref.AP(Playernum));
        }

        // Create planet table for this star
        tabulate::Table table;
        table.format().hide_border().column_separator("  ");

        // Configure columns
        table.column(0).format().width(3).font_align(
            tabulate::FontAlign::right);     // #
        table.column(1).format().width(15);  // Planet
        table.column(2).format().width(30);  // Attributes
        table.column(3).format().width(12);  // Type
        table.column(4).format().width(6).font_align(
            tabulate::FontAlign::right);  // Compat

        // Add header
        table.add_row({"#", "Planet", "Attributes", "Type", "Compat"});
        table[0].format().font_style({tabulate::FontStyle::bold});

        for (const Planet& pl : PlanetList::readonly(
                 g.entity_manager, star_ref.star_id(), star_ref)) {
          const planetnum_t i = pl.planet_order();

          // Build attributes string
          std::string attrs;
          std::string type_col;
          std::string compat_col;
          if (pl.info(Playernum).explored) {
            if (pl.info(Playernum).explored) attrs += "Ex ";
            if (pl.info(Playernum).autorep) attrs += "Rep ";
            if (pl.info(Playernum).numsectsowned) attrs += "Inhab ";
            if (pl.slaved_to() != 0) attrs += "SLAVED ";

            for (player_t j{1}; j <= g.entity_manager.num_races(); ++j) {
              if (j != Playernum && pl.info(j).numsectsowned) {
                attrs += std::format("{} ", j);
              }
            }
            if (pl.conditions(TOXIC) > 70) attrs += "TOXIC ";

            type_col = Planet_types[pl.type()];
            compat_col = std::format("{:.0f}%", pl.compatibility(*g.race));
          } else {
            attrs = "No Data";
          }

          table.add_row({std::format("{}", i.value + 1),
                         star_ref.get_planet_name(i), attrs, type_col,
                         compat_col});
        }

        g.out << table << "\n";
      }
    }
  }
  return true;
}

const CommandDescriptor explore_cmd{
    .name = "explore",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "explore [<where>]",
    .description =
        "Global census and exploration survey of known stars and worlds",
    .handler = &explore,
};

}  // namespace GB::commands
