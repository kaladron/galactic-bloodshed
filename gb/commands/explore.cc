// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

namespace GB::commands {
void explore(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  int starq = -1;

  if (argv.size() == 2) {
    Place where{g, argv[1]};
    if (where.err) {
      g.out << "explore: bad scope.\n";
      return;
    }
    if (where.level == ScopeLevel::LEVEL_SHIP ||
        where.level == ScopeLevel::LEVEL_UNIV) {
      g.out << std::format("Bad scope '{}'\n", argv[1]);
      return;
    }
    starq = where.snum;
  }

  const auto& sdata = *g.entity_manager.peek_universe();
  g.out << "         ========== Exploration Report ==========\n";
  g.out << std::format(" Global action points : [{:2}]\n",
                       sdata.AP[Playernum - 1]);

  for (auto star_handle : StarList(g.entity_manager)) {
    const auto& star_ref = *star_handle;
    if ((starq == -1) || (starq == star_ref.star_id())) {
      if (isset(star_ref.explored(), Playernum)) {
        // Output star header
        if (g.race->tech >= TECH_SEE_STABILITY) {
          g.out << std::format("\n{} ({:2})[{:2}]\n", star_ref.get_name(),
                               star_ref.stability(),
                               star_ref.AP(Playernum - 1));
        } else {
          g.out << std::format("\n{} (/?/?)[{:2}]\n", star_ref.get_name(),
                               star_ref.AP(Playernum - 1));
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

        for (planetnum_t i = 0; i < star_ref.numplanets(); i++) {
          const auto& pl = *g.entity_manager.peek_planet(star_ref.star_id(), i);

          // Build attributes string
          std::string attrs;
          std::string type_col;
          std::string compat_col;
          if (pl.info(Playernum - 1).explored) {
            if (pl.info(Playernum - 1).explored) attrs += "Ex ";
            if (pl.info(Playernum - 1).autorep) attrs += "Rep ";
            if (pl.info(Playernum - 1).numsectsowned) attrs += "Inhab ";
            if (pl.slaved_to()) attrs += "SLAVED ";

            for (int j = 1; j <= g.entity_manager.num_races(); j++) {
              if (j != Playernum && pl.info(j - 1).numsectsowned) {
                attrs += std::format("{} ", j);
              }
            }
            if (pl.conditions(TOXIC) > 70) attrs += "TOXIC ";

            type_col = Planet_types[pl.type()];
            compat_col = std::format("{:.0f}%", pl.compatibility(*g.race));
          } else {
            attrs = "No Data";
          }

          table.add_row({std::format("{}", i + 1), star_ref.get_planet_name(i),
                         attrs, type_col, compat_col});
        }

        g.out << table << "\n";
      }
    }
  }
}
}  // namespace GB::commands
