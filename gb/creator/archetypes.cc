// SPDX-License-Identifier: Apache-2.0

/// \file archetypes.cc
/// \brief Preset racial archetypes and tabulation for player enrollment.

module;

import std;
import tabulate;
import gb.entities;

module gb.creator;

namespace GB::creator {

RaceEnrollmentSpec
RaceArchetype::to_enrollment_spec(std::optional<PlanetType> planet_override,
                                  bool randomize) const {
  PlanetType ptype = planet_override.value_or(default_planet);
  auto [pref, compat] = default_sector_compatibilities_for_planet(ptype);

  RaceEnrollmentSpec spec{};
  spec.name = std::string(name);
  spec.home_planet_type = ptype;
  spec.preferred_sector = pref;
  spec.likesbest = pref;
  spec.sector_compatibilities = compat;

  spec.metamorph = is_metamorphic;
  spec.absorb = is_metamorphic;
  spec.collective_iq = is_metamorphic;
  spec.pods = is_metamorphic;

  if (randomize) {
    spec.mass = sample_mass();
    spec.birthrate = sample_birthrate();
    spec.fighters = sample_fighters();
    spec.iq = sample_iq();
    spec.iq_limit = sample_iq_limit();
    spec.adventurism = sample_adventurism();
    spec.number_sexes = sample_sexes();
    spec.metabolism = sample_metabolism();
  } else {
    spec.mass = base_mass;
    spec.birthrate = base_birthrate;
    spec.fighters = base_fighters;
    spec.iq = is_metamorphic ? 0 : base_iq;
    spec.iq_limit = is_metamorphic ? base_iq_limit : 0;
    spec.adventurism = base_adventurism;
    spec.number_sexes = min_sexes;
    spec.metabolism = base_metabolism;
  }
  return spec;
}

tabulate::Table create_archetypes_table() {
  tabulate::Table table;
  table.add_row({"#", "Archetype", "Planet", "Mass", "Birth", "Fight", "IQ",
                 "Metab", "Advent", "Sexes", "Special Traits"});

  for (std::size_t i = 0; i < race_archetypes.size(); ++i) {
    const auto& arch = race_archetypes[i];
    std::string sexes_str =
        (arch.min_sexes == arch.max_sexes)
            ? std::format("{}", arch.min_sexes)
            : std::format("{}-{}", arch.min_sexes, arch.max_sexes);
    std::string iq_str = arch.is_metamorphic
                             ? std::format("Col ({})", arch.base_iq_limit)
                             : std::format("{}", arch.base_iq);
    std::string traits_str =
        arch.is_metamorphic ? "Metamorph, Absorb, Pods" : "Standard";

    table.add_row({
        std::format("{}", i + 1),
        std::string(arch.name),
        std::string(to_string(arch.default_planet)),
        std::format("{:.2f}", arch.base_mass),
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
  for (std::size_t c = 0; c < table[0].size(); ++c) {
    table[0][c].format().font_style({tabulate::FontStyle::bold});
  }

  // Align numeric and structured columns
  for (std::size_t r = 1; r <= race_archetypes.size(); ++r) {
    table[r][0].format().font_align(tabulate::FontAlign::right);
    table[r][3].format().font_align(tabulate::FontAlign::right);
    table[r][4].format().font_align(tabulate::FontAlign::right);
    table[r][5].format().font_align(tabulate::FontAlign::right);
    table[r][6].format().font_align(tabulate::FontAlign::right);
    table[r][7].format().font_align(tabulate::FontAlign::right);
    table[r][8].format().font_align(tabulate::FontAlign::right);
    table[r][9].format().font_align(tabulate::FontAlign::center);
  }

  return table;
}

}  // namespace GB::creator
