// SPDX-License-Identifier: Apache-2.0

/// \file profile.cc
/// \brief Display racial profile, stats, morale, and planetary conditions.

module;

import gb.entities;
import gb.services;
import std;
import tabulate;
#undef stdout

module commands;

namespace GB::commands {
bool profile(const command_t& argv, GameObj& g) {
  const auto& race = *g.race;

  // Get information about ourselves
  if (argv.size() == 1) {
    g.out << std::format("--==** Racial profile for {} (player {}) **==--\n",
                         race.name, race.Playernum);
    if (race.God) {
      g.out << "*** Diety Status ***\n";
    }
    g.out << std::format("Personal: {}\n", race.info);
    const auto& homestar = *g.entity_manager.peek_star(
        race.governor[g.governor().value].homesystem);
    g.out << std::format("Default Scope: /{}/{}\n", homestar.get_name(),
                         homestar.get_planet_name(
                             race.governor[g.governor().value].homeplanetnum));
    if (race.Gov_ship == 0)
      g.out << "NO DESIGNATED CAPITAL!!\n";
    else
      g.out << std::format("Designated Capital: #{}\n", race.Gov_ship);
    g.out << std::format("Morale: {}\n", race.morale);
    g.out << std::format("Updates active: {}\n", race.turn);
    g.out << "Ranges:\n";
    g.out << std::format("  guns:   {:.2f}\n", race.gun_range());
    g.out << std::format("  space:  {:.2f}\n",
                         tele_range(ShipType::OTYPE_STELE, race.tech));
    g.out << std::format("  ground: {:.2f}\n\n",
                         tele_range(ShipType::OTYPE_GTELE, race.tech));

    // Race characteristics and planet conditions table
    g.out << std::format("{}\n\n",
                         race.Metamorph ? "Metamorphic Race" : "Normal Race");

    tabulate::Table race_table;
    race_table.format().hide_border().column_separator("  ");

    // Configure column alignments
    race_table.column(0).format().width(10);  // Stat name
    race_table.column(1).format().width(12).font_align(
        tabulate::FontAlign::right);          // Stat value
    race_table.column(2).format().width(3);   // Spacer
    race_table.column(3).format().width(10);  // Condition name
    race_table.column(4).format().width(8).font_align(
        tabulate::FontAlign::right);          // Condition value
    race_table.column(5).format().width(3);   // Spacer
    race_table.column(6).format().width(12);  // Sector name
    race_table.column(7).format().width(3).font_align(
        tabulate::FontAlign::center);  // Sector char
    race_table.column(8).format().width(8).font_align(
        tabulate::FontAlign::right);  // Sector preference

    // Add header row
    race_table.add_row(
        {"", "", "", "Planet", "Conditions", "", "Sector", "Preferences", ""});
    race_table[0].format().font_style({tabulate::FontStyle::bold});

    // Add data rows with proper alignment
    race_table.add_row({"Fert:", std::format("{}%", race.fertilize), "",
                        "Temp:", std::format("{}", race.conditions[TEMP]), "",
                        "", "", ""});
    race_table.add_row(
        {"Rate:", std::format("{:.2f}", race.birthrate), "",
         "methane:", std::format("{}%", race.conditions[METHANE]), "",
         Desnames[SectorType::SEC_SEA], std::format("{}", CHAR_SEA),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_SEA)))});
    race_table.add_row(
        {"Mass:", std::format("{:.3f}", race.mass), "",
         "oxygen:", std::format("{}%", race.conditions[OXYGEN]), "",
         Desnames[SectorType::SEC_GAS], std::format("{}", CHAR_GAS),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_GAS)))});
    race_table.add_row(
        {"Fight:", std::format("{}", race.fighters), "",
         "helium:", std::format("{}%", race.conditions[HELIUM]), "",
         Desnames[SectorType::SEC_ICE], std::format("{}", CHAR_ICE),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_ICE)))});
    race_table.add_row(
        {"Metab:", std::format("{:.2f}", race.metabolism), "",
         "nitrogen:", std::format("{}%", race.conditions[NITROGEN]), "",
         Desnames[SectorType::SEC_MOUNT], std::format("{}", CHAR_MOUNT),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_MOUNT)))});
    race_table.add_row(
        {"Sexes:", std::format("{}", race.number_sexes), "",
         "CO2:", std::format("{}%", race.conditions[CO2]), "",
         Desnames[SectorType::SEC_LAND], std::format("{}", CHAR_LAND),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_LAND)))});
    race_table.add_row(
        {"Explore:",
         std::format("{:.0f}%", fraction_to_percent(race.adventurism)), "",
         "hydrogen:", std::format("{}%", race.conditions[HYDROGEN]), "",
         Desnames[SectorType::SEC_DESERT], std::format("{}", CHAR_DESERT),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_DESERT)))});
    race_table.add_row(
        {"Avg Int:", std::format("{}", race.IQ), "",
         "sulfur:", std::format("{}%", race.conditions[SULFUR]), "",
         Desnames[SectorType::SEC_FOREST], std::format("{}", CHAR_FOREST),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_FOREST)))});
    race_table.add_row(
        {"Tech:", std::format("{:.2f}", race.tech), "",
         "other:", std::format("{}%", race.conditions[OTHER]), "",
         Desnames[SectorType::SEC_PLATED], std::format("{}", CHAR_PLATED),
         std::format("{:.0f}", fraction_to_percent(race.sector_compatibility(
                                   SectorType::SEC_PLATED)))});

    g.out << race_table << "\n\n";

    g.out << "Discoveries:";
    if (race.discoveries.crystal) g.out << "  Crystals";
    if (race.discoveries.hyperdrive) g.out << "  Hyper-drive";
    if (race.discoveries.laser) g.out << "  Combat Lasers";
    if (race.discoveries.cew) g.out << "  Confined Energy Weapons";
    if (race.discoveries.vn) g.out << "  Von Neumann Machines";
    if (race.discoveries.tractor_beam) g.out << "  Tractor Beam";
    if (race.discoveries.transporter) g.out << "  Transporter";
    if (race.discoveries.avpm) g.out << "  AVPM";
    if (race.discoveries.cloak) g.out << "  Cloaking";
    if (race.discoveries.wormhole) g.out << "  Wormhole";
    g.out << "\n";
    return true;
  }

  // Get information about another player.
  player_t p = get_player(g.entity_manager, argv[1]);
  if (p == 0) {
    g.out << "Player does not exist.\n";
    return false;
  }
  try {
    g.entity_manager.with_race(p, [&](const Race& r) {
      g.out << std::format("------ Race report on {} ({}) ------\n", r.name, p);
      if (race.God && r.God) {
        g.out << "*** Deity Status ***\n";
      }
      g.out << std::format("Personal: {}\n", r.info);
      g.out << std::format("%%Know:  {}%\n", race.translate[p]);
      if (race.translate[p] > 50) {
        g.out << std::format("{}\t  Planet Conditions\n",
                             r.Metamorph ? "Metamorphic Race"
                                         : "Normal Race\t");
        g.out << std::format("Fert:    {}", race.estimate(r.fertilize, p));
        g.out << std::format("\t\t  Temp:\t{}\n",
                             race.estimate(r.conditions[TEMP], p));
        g.out << std::format("Rate:    {}%%",
                             race.estimate(r.birthrate * 100.0, p));
      } else {
        g.out << "Unknown Race\t\t  Planet Conditions\n";
        g.out << std::format("Fert:    {}", race.estimate(r.fertilize, p));
        g.out << std::format("\t\t  Temp:\t{}\n",
                             race.estimate(r.conditions[TEMP], p));
        g.out << std::format("Rate:    {}", race.estimate(r.birthrate, p));
      }
      g.out << std::format("\t\t  methane  {}%\t\tRanges:\n",
                           race.estimate(r.conditions[METHANE], p));
      g.out << std::format("Mass:    {}", race.estimate(r.mass, p));
      g.out << std::format("\t\t  oxygen   {}%",
                           race.estimate(r.conditions[OXYGEN], p));
      g.out << std::format("\t\t  guns:   {}\n",
                           race.estimate(r.gun_range(), p));
      g.out << std::format("Fight:   {}", race.estimate(r.fighters, p));
      g.out << std::format("\t\t  helium   {}%",
                           race.estimate(r.conditions[HELIUM], p));
      g.out << std::format(
          "\t\t  space:  {}\n",
          race.estimate(tele_range(ShipType::OTYPE_STELE, r.tech), p));
      g.out << std::format("Metab:   {}", race.estimate(r.metabolism, p));
      g.out << std::format("\t\t  nitrogen {}%",
                           race.estimate(r.conditions[NITROGEN], p));
      g.out << std::format(
          "\t\t  ground: {}\n",
          race.estimate(tele_range(ShipType::OTYPE_GTELE, r.tech), p));
      g.out << std::format("Sexes:   {}", race.estimate(r.number_sexes, p));
      g.out << std::format("\t\t  CO2      {}%\n",
                           race.estimate(r.conditions[CO2], p));
      g.out << std::format("Explore: {}%",
                           race.estimate(r.adventurism * 100.0, p));
      g.out << std::format("\t\t  hydrogen {}%\n",
                           race.estimate(r.conditions[HYDROGEN], p));
      g.out << std::format("Avg Int: {}", race.estimate(r.IQ, p));
      g.out << std::format("\t\t  sulfer   {}%\n",
                           race.estimate(r.conditions[SULFUR], p));
      g.out << std::format("Tech:    {}", race.estimate(r.tech, p));
      g.out << std::format("\t\t  other    {}%",
                           race.estimate(r.conditions[OTHER], p));
      g.out << std::format("\t\tMorale:   {}\n", race.estimate(r.morale, p));
      g.out << std::format("Sector type preference : {}\n",
                           race.translate[p] > 80 ? Desnames[r.likesbest]
                                                  : " ? ");
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Race not found.\n";
    return false;
  }
  return true;
}

const CommandDescriptor profile_cmd{
    .name = "profile",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "profile [<race name>]",
    .description =
        "Display racial profile, stats, morale, and planetary conditions",
    .handler = &profile,
};

}  // namespace GB::commands
