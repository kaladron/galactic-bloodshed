// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void profile(const command_t &argv, GameObj &g) {
  auto &race = races[g.player - 1];

  // Get information about ourselves
  if (argv.size() == 1) {
    g.out << std::format("--==** Racial profile for {} (player {}) **==--\n",
                         race.name, race.Playernum);
    if (race.God) {
      g.out << "*** Diety Status ***\n";
    }
    g.out << std::format("Personal: {}\n", race.info);
    g.out << std::format(
        "Default Scope: /{}/{}\n",
        stars[race.governor[g.governor].homesystem].get_name(),
        stars[race.governor[g.governor].homesystem].get_planet_name(
            race.governor[g.governor].homeplanetnum));
    if (race.Gov_ship == 0)
      g.out << "NO DESIGNATED CAPITAL!!";
    else
      g.out << std::format(
          "Designated Capital: #{}    Ranges:     guns:   {}\n", race.Gov_ship,
          gun_range(race));
    g.out << std::format("Morale: {}    space:  {}\n", race.morale,
                         tele_range(ShipType::OTYPE_STELE, race.tech));
    g.out << std::format("Updates active: {}    ground: {}\n\n", race.turn,
                         tele_range(ShipType::OTYPE_GTELE, race.tech));
    g.out << std::format(
        "{}  Planet Conditions\t      Sector Preferences\n",
        race.Metamorph ? "Metamorphic Race\t" : "Normal Race\t\t");
    g.out << std::format("Fert:    {}%\t\t  Temp:\t{}\n", race.fertilize,
                         race.conditions[TEMP]);
    g.out << std::format("Rate:    {}\t\t  methane  {}%\t      {} {} {}\n",
                         race.birthrate, race.conditions[METHANE],
                         Desnames[SectorType::SEC_SEA], CHAR_SEA,
                         race.likes[SectorType::SEC_SEA] * 100.);
    g.out << std::format("Mass:    {}\t\t  oxygen   {}%\t      {} {} {}\n",
                         race.mass, race.conditions[OXYGEN],
                         Desnames[SectorType::SEC_GAS], CHAR_GAS,
                         race.likes[SectorType::SEC_GAS] * 100.);
    g.out << std::format("Fight:   {}\t\t  helium   {}%\t      {} {} {}\n",
                         race.fighters, race.conditions[HELIUM],
                         Desnames[SectorType::SEC_ICE], CHAR_ICE,
                         race.likes[SectorType::SEC_ICE] * 100.);
    g.out << std::format("Metab:   {}\t\t  nitrogen {}%\t      {} {} {}\n",
                         race.metabolism, race.conditions[NITROGEN],
                         Desnames[SectorType::SEC_MOUNT], CHAR_MOUNT,
                         race.likes[SectorType::SEC_MOUNT] * 100.);
    g.out << std::format("Sexes:   {}\t\t  CO2      {}%\t      {} {} {}\n",
                         race.number_sexes, race.conditions[CO2],
                         Desnames[SectorType::SEC_LAND], CHAR_LAND,
                         race.likes[SectorType::SEC_LAND] * 100.);
    g.out << std::format("Explore: {}%\t\t  hydrogen {}%\t      {} {} {}\n",
                         race.adventurism * 100.0, race.conditions[HYDROGEN],
                         Desnames[SectorType::SEC_DESERT], CHAR_DESERT,
                         race.likes[SectorType::SEC_DESERT] * 100.);
    g.out << std::format("Avg Int: {}\t\t  sulfer   {}%\t      {} {} {}\n",
                         race.IQ, race.conditions[SULFUR],
                         Desnames[SectorType::SEC_FOREST], CHAR_FOREST,
                         race.likes[SectorType::SEC_FOREST] * 100.);
    g.out << std::format("Tech:    {}\t\t  other    {}%\t      {} {} {}\n",
                         race.tech, race.conditions[OTHER],
                         Desnames[SectorType::SEC_PLATED], CHAR_PLATED,
                         race.likes[SectorType::SEC_PLATED] * 100.);

    g.out << "Discoveries:";
    if (Crystal(race)) g.out << "  Crystals";
    if (Hyper_drive(race)) g.out << "  Hyper-drive";
    if (Laser(race)) g.out << "  Combat Lasers";
    if (Cew(race)) g.out << "  Confined Energy Weapons";
    if (Vn(race)) g.out << "  Von Neumann Machines";
    if (Tractor_beam(race)) g.out << "  Tractor Beam";
    if (Transporter(race)) g.out << "  Transporter";
    if (Avpm(race)) g.out << "  AVPM";
    if (Cloak(race)) g.out << "  Cloaking";
    if (Wormhole(race)) g.out << "  Wormhole";
    g.out << "\n";
    return;
  }

  // Get information about another player.
  player_t p = get_player(argv[1]);
  if (p == 0) {
    g.out << "Player does not exist.\n";
    return;
  }
  auto &r = races[p - 1];
  g.out << std::format("------ Race report on {} ({}) ------\n", r.name, p);
  if (race.God && r.God) {
    g.out << "*** Deity Status ***\n";
  }
  g.out << std::format("Personal: {}\n", r.info);
  g.out << std::format("%%Know:  {}%\n", race.translate[p - 1]);
  if (race.translate[p - 1] > 50) {
    g.out << std::format("{}\t  Planet Conditions\n",
                         r.Metamorph ? "Metamorphic Race" : "Normal Race\t");
    g.out << std::format("Fert:    {}",
                         Estimate_i((int)(r.fertilize), race, p));
    g.out << std::format("\t\t  Temp:\t{}\n",
                         Estimate_i((int)(r.conditions[TEMP]), race, p));
    g.out << std::format("Rate:    {}%%",
                         Estimate_f(r.birthrate * 100.0, race, p));
  } else {
    g.out << "Unknown Race\t\t  Planet Conditions\n";
    g.out << std::format("Fert:    {}",
                         Estimate_i((int)(r.fertilize), race, p));
    g.out << std::format("\t\t  Temp:\t{}\n",
                         Estimate_i((int)(r.conditions[TEMP]), race, p));
    g.out << std::format("Rate:    {}", Estimate_f(r.birthrate, race, p));
  }
  g.out << std::format("\t\t  methane  {}%\t\tRanges:\n",
                       Estimate_i((int)(r.conditions[METHANE]), race, p));
  g.out << std::format("Mass:    {}", Estimate_f(r.mass, race, p));
  g.out << std::format("\t\t  oxygen   {}%",
                       Estimate_i((int)(r.conditions[OXYGEN]), race, p));
  g.out << std::format("\t\t  guns:   {}\n", Estimate_f(gun_range(r), race, p));
  g.out << std::format("Fight:   {}", Estimate_i((int)(r.fighters), race, p));
  g.out << std::format("\t\t  helium   {}%",
                       Estimate_i((int)(r.conditions[HELIUM]), race, p));
  g.out << std::format(
      "\t\t  space:  {}\n",
      Estimate_f(tele_range(ShipType::OTYPE_STELE, r.tech), race, p));
  g.out << std::format("Metab:   {}", Estimate_f(r.metabolism, race, p));
  g.out << std::format("\t\t  nitrogen {}%",
                       Estimate_i((int)(r.conditions[NITROGEN]), race, p));
  g.out << std::format(
      "\t\t  ground: {}\n",
      Estimate_f(tele_range(ShipType::OTYPE_GTELE, r.tech), race, p));
  g.out << std::format("Sexes:   {}",
                       Estimate_i((int)(r.number_sexes), race, p));
  g.out << std::format("\t\t  CO2      {}%\n",
                       Estimate_i((int)(r.conditions[CO2]), race, p));
  g.out << std::format("Explore: {}%",
                       Estimate_f(r.adventurism * 100.0, race, p));
  g.out << std::format("\t\t  hydrogen {}%\n",
                       Estimate_i((int)(r.conditions[HYDROGEN]), race, p));
  g.out << std::format("Avg Int: {}", Estimate_i((int)(r.IQ), race, p));
  g.out << std::format("\t\t  sulfer   {}%\n",
                       Estimate_i((int)(r.conditions[SULFUR]), race, p));
  g.out << std::format("Tech:    {}", Estimate_f(r.tech, race, p));
  g.out << std::format("\t\t  other    {}%",
                       Estimate_i((int)(r.conditions[OTHER]), race, p));
  g.out << std::format("\t\tMorale:   {}\n",
                       Estimate_i((int)(r.morale), race, p));
  g.out << std::format(
      "Sector type preference : {}\n",
      race.translate[p - 1] > 80 ? Desnames[r.likesbest] : " ? ");
}
}  // namespace GB::commands
