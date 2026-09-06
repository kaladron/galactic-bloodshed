// SPDX-License-Identifier: Apache-2.0

/// \file archetypes.cc
/// \brief Preset racial archetypes and tabulation for player enrollment.

module;

import std;
import tabulate;
import gb.entities;

module gb.creator;

namespace GB::creator {

tabulate::Table create_archetypes_table() {
  tabulate::Table table;
  table.add_row({"#", "Archetype", "Mass", "Birth", "Fight", "IQ", "Metab",
                 "Advent", "Sexes", "Special Traits"});

  for (std::size_t i = 0; i < race_archetypes.size(); ++i) {
    const auto& arch = race_archetypes[i];
    std::string sexes_str =
        (arch.min_sexes == arch.max_sexes)
            ? std::format("{}", arch.min_sexes)
            : std::format("{}-{}", arch.min_sexes, arch.max_sexes);
    std::string iq_str =
        arch.is_metamorphic ? "Col (0)" : std::format("{}", arch.base_iq);
    std::string traits_str =
        arch.is_metamorphic ? "Metamorph, Absorb, Pods" : "Standard";

    table.add_row({
        std::format("{}", i + 1),
        std::string(arch.name),
        std::format("{:.3f}", arch.base_mass),
        std::format("{:.2f}", arch.base_birthrate),
        std::format("{}", arch.base_fighters),
        iq_str,
        std::format("{:.2f}", arch.base_metabolism),
        std::format("{:.2f}", arch.base_adventurism),
        sexes_str,
        traits_str,
    });
  }

  // Format header row style
  for (std::size_t c = 0; c < 10; ++c) {
    table[0][c].format().font_style({tabulate::FontStyle::bold});
  }

  // Align numeric and structured columns
  for (std::size_t r = 1; r <= race_archetypes.size(); ++r) {
    table[r][0].format().font_align(tabulate::FontAlign::right);
    table[r][2].format().font_align(tabulate::FontAlign::right);
    table[r][3].format().font_align(tabulate::FontAlign::right);
    table[r][4].format().font_align(tabulate::FontAlign::right);
    table[r][5].format().font_align(tabulate::FontAlign::right);
    table[r][6].format().font_align(tabulate::FontAlign::right);
    table[r][7].format().font_align(tabulate::FontAlign::right);
    table[r][8].format().font_align(tabulate::FontAlign::center);
  }

  return table;
}

}  // namespace GB::creator
