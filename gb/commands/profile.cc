// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;
import tabulate;

module commands;

namespace GB::commands {
void profile(const command_t& argv, GameObj& g) {
  const auto* race = g.entity_manager.peek_race(g.player);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  // Get information about ourselves
  if (argv.size() == 1) {
    g.out << std::format("--==** Racial profile for {} (player {}) **==--\n",
                         race->name, race->Playernum);
    if (race->God) {
      g.out << "*** Diety Status ***\n";
    }
    g.out << std::format("Personal: {}\n", race->info);
    g.out << std::format(
        "Default Scope: /{}/{}\n",
        stars[race->governor[g.governor].homesystem].get_name(),
        stars[race->governor[g.governor].homesystem].get_planet_name(
            race->governor[g.governor].homeplanetnum));
    if (race->Gov_ship == 0)
      g.out << "NO DESIGNATED CAPITAL!!\n";
    else
      g.out << std::format("Designated Capital: #{}\n", race->Gov_ship);
    g.out << std::format("Morale: {}\n", race->morale);
    g.out << std::format("Updates active: {}\n", race->turn);
    g.out << "Ranges:\n";
    g.out << std::format("  guns:   {:.2f}\n", gun_range(*race));
    g.out << std::format("  space:  {:.2f}\n",
                         tele_range(ShipType::OTYPE_STELE, race->tech));
    g.out << std::format("  ground: {:.2f}\n\n",
                         tele_range(ShipType::OTYPE_GTELE, race->tech));

    // Race characteristics and planet conditions table
    g.out << std::format("{}\n\n",
                         race->Metamorph ? "Metamorphic Race" : "Normal Race");

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
    race_table.add_row({"Fert:", std::format("{}%", race->fertilize), "",
                        "Temp:", std::format("{}", race->conditions[TEMP]), "",
                        "", "", ""});
    race_table.add_row(
        {"Rate:", std::format("{:.2f}", race->birthrate), "",
         "methane:", std::format("{}%", race->conditions[METHANE]), "",
         Desnames[SectorType::SEC_SEA], std::format("{}", CHAR_SEA),
         std::format("{:.0f}", race->likes[SectorType::SEC_SEA] * 100.)});
    race_table.add_row(
        {"Mass:", std::format("{:.3f}", race->mass), "",
         "oxygen:", std::format("{}%", race->conditions[OXYGEN]), "",
         Desnames[SectorType::SEC_GAS], std::format("{}", CHAR_GAS),
         std::format("{:.0f}", race->likes[SectorType::SEC_GAS] * 100.)});
    race_table.add_row(
        {"Fight:", std::format("{}", race->fighters), "",
         "helium:", std::format("{}%", race->conditions[HELIUM]), "",
         Desnames[SectorType::SEC_ICE], std::format("{}", CHAR_ICE),
         std::format("{:.0f}", race->likes[SectorType::SEC_ICE] * 100.)});
    race_table.add_row(
        {"Metab:", std::format("{:.2f}", race->metabolism), "",
         "nitrogen:", std::format("{}%", race->conditions[NITROGEN]), "",
         Desnames[SectorType::SEC_MOUNT], std::format("{}", CHAR_MOUNT),
         std::format("{:.0f}", race->likes[SectorType::SEC_MOUNT] * 100.)});
    race_table.add_row(
        {"Sexes:", std::format("{}", race->number_sexes), "",
         "CO2:", std::format("{}%", race->conditions[CO2]), "",
         Desnames[SectorType::SEC_LAND], std::format("{}", CHAR_LAND),
         std::format("{:.0f}", race->likes[SectorType::SEC_LAND] * 100.)});
    race_table.add_row(
        {"Explore:", std::format("{:.0f}%", race->adventurism * 100.0), "",
         "hydrogen:", std::format("{}%", race->conditions[HYDROGEN]), "",
         Desnames[SectorType::SEC_DESERT], std::format("{}", CHAR_DESERT),
         std::format("{:.0f}", race->likes[SectorType::SEC_DESERT] * 100.)});
    race_table.add_row(
        {"Avg Int:", std::format("{}", race->IQ), "",
         "sulfur:", std::format("{}%", race->conditions[SULFUR]), "",
         Desnames[SectorType::SEC_FOREST], std::format("{}", CHAR_FOREST),
         std::format("{:.0f}", race->likes[SectorType::SEC_FOREST] * 100.)});
    race_table.add_row(
        {"Tech:", std::format("{:.2f}", race->tech), "",
         "other:", std::format("{}%", race->conditions[OTHER]), "",
         Desnames[SectorType::SEC_PLATED], std::format("{}", CHAR_PLATED),
         std::format("{:.0f}", race->likes[SectorType::SEC_PLATED] * 100.)});

    g.out << race_table << "\n\n";

    g.out << "Discoveries:";
    if (Crystal(*race)) g.out << "  Crystals";
    if (Hyper_drive(*race)) g.out << "  Hyper-drive";
    if (Laser(*race)) g.out << "  Combat Lasers";
    if (Cew(*race)) g.out << "  Confined Energy Weapons";
    if (Vn(*race)) g.out << "  Von Neumann Machines";
    if (Tractor_beam(*race)) g.out << "  Tractor Beam";
    if (Transporter(*race)) g.out << "  Transporter";
    if (Avpm(*race)) g.out << "  AVPM";
    if (Cloak(*race)) g.out << "  Cloaking";
    if (Wormhole(*race)) g.out << "  Wormhole";
    g.out << "\n";
    return;
  }

  // Get information about another player.
  player_t p = get_player(g.entity_manager, argv[1]);
  if (p == 0) {
    g.out << "Player does not exist.\n";
    return;
  }
  auto& r = races[p - 1];
  g.out << std::format("------ Race report on {} ({}) ------\n", r.name, p);
  if (race->God && r.God) {
    g.out << "*** Deity Status ***\n";
  }
  g.out << std::format("Personal: {}\n", r.info);
  g.out << std::format("%%Know:  {}%\n", race->translate[p - 1]);
  if (race->translate[p - 1] > 50) {
    g.out << std::format("{}\t  Planet Conditions\n",
                         r.Metamorph ? "Metamorphic Race" : "Normal Race\t");
    g.out << std::format("Fert:    {}",
                         Estimate_i((int)(r.fertilize), *race, p));
    g.out << std::format("\t\t  Temp:\t{}\n",
                         Estimate_i((int)(r.conditions[TEMP]), *race, p));
    g.out << std::format("Rate:    {}%%",
                         Estimate_f(r.birthrate * 100.0, *race, p));
  } else {
    g.out << "Unknown Race\t\t  Planet Conditions\n";
    g.out << std::format("Fert:    {}",
                         Estimate_i((int)(r.fertilize), *race, p));
    g.out << std::format("\t\t  Temp:\t{}\n",
                         Estimate_i((int)(r.conditions[TEMP]), *race, p));
    g.out << std::format("Rate:    {}", Estimate_f(r.birthrate, *race, p));
  }
  g.out << std::format("\t\t  methane  {}%\t\tRanges:\n",
                       Estimate_i((int)(r.conditions[METHANE]), *race, p));
  g.out << std::format("Mass:    {}", Estimate_f(r.mass, *race, p));
  g.out << std::format("\t\t  oxygen   {}%",
                       Estimate_i((int)(r.conditions[OXYGEN]), *race, p));
  g.out << std::format("\t\t  guns:   {}\n",
                       Estimate_f(gun_range(r), *race, p));
  g.out << std::format("Fight:   {}", Estimate_i((int)(r.fighters), *race, p));
  g.out << std::format("\t\t  helium   {}%",
                       Estimate_i((int)(r.conditions[HELIUM]), *race, p));
  g.out << std::format(
      "\t\t  space:  {}\n",
      Estimate_f(tele_range(ShipType::OTYPE_STELE, r.tech), *race, p));
  g.out << std::format("Metab:   {}", Estimate_f(r.metabolism, *race, p));
  g.out << std::format("\t\t  nitrogen {}%",
                       Estimate_i((int)(r.conditions[NITROGEN]), *race, p));
  g.out << std::format(
      "\t\t  ground: {}\n",
      Estimate_f(tele_range(ShipType::OTYPE_GTELE, r.tech), *race, p));
  g.out << std::format("Sexes:   {}",
                       Estimate_i((int)(r.number_sexes), *race, p));
  g.out << std::format("\t\t  CO2      {}%\n",
                       Estimate_i((int)(r.conditions[CO2]), *race, p));
  g.out << std::format("Explore: {}%",
                       Estimate_f(r.adventurism * 100.0, *race, p));
  g.out << std::format("\t\t  hydrogen {}%\n",
                       Estimate_i((int)(r.conditions[HYDROGEN]), *race, p));
  g.out << std::format("Avg Int: {}", Estimate_i((int)(r.IQ), *race, p));
  g.out << std::format("\t\t  sulfer   {}%\n",
                       Estimate_i((int)(r.conditions[SULFUR]), *race, p));
  g.out << std::format("Tech:    {}", Estimate_f(r.tech, *race, p));
  g.out << std::format("\t\t  other    {}%",
                       Estimate_i((int)(r.conditions[OTHER]), *race, p));
  g.out << std::format("\t\tMorale:   {}\n",
                       Estimate_i((int)(r.morale), *race, p));
  g.out << std::format("Sector type preference : {}\n",
                       race->translate[p - 1] > 80 ? Desnames[r.likesbest]
                                                   : " ? ");
}
}  // namespace GB::commands
