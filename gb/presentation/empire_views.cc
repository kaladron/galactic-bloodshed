// SPDX-License-Identifier: Apache-2.0

/// \file empire_views.cc
/// \brief ASCII and Glaze JSON renderers for empire and diplomatic view models.

module gb.presentation.empire_views;

import gb.entities;
import gb.services;
import tabulate;
import std;

namespace GB::presentation {

std::string render_relation(const RelationViewModel& vm) {
  std::string out = std::format(
      "\n              Racial Relations Report for {}\n\n"
      " #       know             Race name       Yours        Theirs\n"
      " -       ----             ---------       -----        ------\n",
      vm.subject_race_name);
  for (const auto& row : vm.rows) {
    std::format_to(std::back_inserter(out),
                   "{:2} {:5} ({:3d}%) {:>20.20} : {:>10}   {:>10}\n",
                   row.player, row.is_known_morph ? "Morph" : "     ", row.know,
                   row.race_name, row.yours, row.theirs);
  }
  return out;
}

std::string render_json_relation(const RelationViewModel& vm) {
  return render_json_envelope("relation", vm);
}

std::string render_for_mode(UiMode mode, const RelationViewModel& vm) {
  return mode == UiMode::JSON ? render_json_relation(vm) : render_relation(vm);
}

std::string render_who(const WhoViewModel& vm) {
  std::string out = std::format("Current Players: {}", std::ctime(&vm.now));

  tabulate::Table table;
  table.add_row({"Race", "Governor", "Player", "Idle", "Star", "Flags"});

  for (const auto& row : vm.rows) {
    const std::string gov_name = std::format("\"{}\"", row.governor_name);
    const std::string player_gov =
        std::format("[{},{}]", row.player, row.governor);
    const std::string idle_str = std::format("{}s", row.idle_seconds);

    std::string flags_str;
    if (row.gag) {
      flags_str += "GAG";
    }
    if (row.invisible) {
      if (!flags_str.empty()) flags_str += " ";
      flags_str += "INVISIBLE";
    }

    table.add_row({row.race_name, gov_name, player_gov, idle_str, row.star_name,
                   flags_str});
  }

  std::format_to(std::back_inserter(out), "{}\n", table.str());

  if (vm.show_cowards) {
    std::format_to(std::back_inserter(out), "And {} coward{}.\n",
                   vm.coward_count, (vm.coward_count == 1) ? "" : "s");
  } else {
    out += "Finished.\n";
  }
  return out;
}

std::string render_json_who(const WhoViewModel& vm) {
  return render_json_envelope("who", vm);
}

std::string render_for_mode(UiMode mode, const WhoViewModel& vm) {
  return mode == UiMode::JSON ? render_json_who(vm) : render_who(vm);
}

std::string render_whois(const WhoisViewModel& vm) {
  std::string out;
  for (const auto& entry : vm.entries) {
    if (entry.error) {
      std::format_to(std::back_inserter(out), "{}\n", *entry.error);
    } else if (entry.governor && entry.governor_name) {
      std::format_to(std::back_inserter(out), "[{:2d}, {}] {} \"{}\"\n",
                     entry.player, *entry.governor, entry.race_name,
                     *entry.governor_name);
    } else {
      std::format_to(std::back_inserter(out), "[{:2d}] {}\n", entry.player,
                     entry.race_name);
    }
  }
  return out;
}

std::string render_json_whois(const WhoisViewModel& vm) {
  return render_json_envelope("whois", vm);
}

std::string render_for_mode(UiMode mode, const WhoisViewModel& vm) {
  return mode == UiMode::JSON ? render_json_whois(vm) : render_whois(vm);
}

std::string render_profile_self(const ProfileSelfViewModel& vm) {
  std::string out =
      std::format("--==** Racial profile for {} (player {}) **==--\n",
                  vm.race_name, vm.player);
  if (vm.is_god) {
    out += "*** Diety Status ***\n";
  }
  std::format_to(std::back_inserter(out),
                 "Personal: {}\n"
                 "Default Scope: /{}/{}\n",
                 vm.personal_info, vm.home_star_name, vm.home_planet_name);
  if (!vm.gov_ship) {
    out += "NO DESIGNATED CAPITAL!!\n";
  } else {
    std::format_to(std::back_inserter(out), "Designated Capital: #{}\n",
                   *vm.gov_ship);
  }
  std::format_to(std::back_inserter(out),
                 "Morale: {}\n"
                 "Updates active: {}\n"
                 "Ranges:\n"
                 "  guns:   {:.2f}\n"
                 "  space:  {:.2f}\n"
                 "  ground: {:.2f}\n\n"
                 "{}\n\n",
                 vm.morale, vm.updates_active, vm.gun_range,
                 vm.space_tele_range, vm.ground_tele_range,
                 vm.is_metamorph ? "Metamorphic Race" : "Normal Race");

  tabulate::Table race_table;
  race_table.format().hide_border().column_separator("  ");
  race_table.column(0).format().width(10);
  race_table.column(1).format().width(12).font_align(
      tabulate::FontAlign::right);
  race_table.column(2).format().width(3);
  race_table.column(3).format().width(10);
  race_table.column(4).format().width(8).font_align(tabulate::FontAlign::right);
  race_table.column(5).format().width(3);
  race_table.column(6).format().width(12);
  race_table.column(7).format().width(3).font_align(
      tabulate::FontAlign::center);
  race_table.column(8).format().width(8).font_align(tabulate::FontAlign::right);

  race_table.add_row(
      {"", "", "", "Planet", "Conditions", "", "Sector", "Preferences", ""});
  race_table[0].format().font_style({tabulate::FontStyle::bold});

  race_table.add_row({"Fert:", std::format("{}%", vm.fertilize), "",
                      "Temp:", std::format("{}", vm.temp), "", "", "", ""});
  race_table.add_row(
      {"Rate:", std::format("{:.2f}", vm.birthrate), "",
       "methane:", std::format("{}%", vm.conditions[METHANE]), "",
       sector_type_name(SectorType::SEC_SEA), std::format("{}", CHAR_SEA),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_SEA]))});
  race_table.add_row(
      {"Mass:", std::format("{:.3f}", vm.mass), "",
       "oxygen:", std::format("{}%", vm.conditions[OXYGEN]), "",
       sector_type_name(SectorType::SEC_GAS), std::format("{}", CHAR_GAS),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_GAS]))});
  race_table.add_row(
      {"Fight:", std::format("{}", vm.fighters), "",
       "helium:", std::format("{}%", vm.conditions[HELIUM]), "",
       sector_type_name(SectorType::SEC_ICE), std::format("{}", CHAR_ICE),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_ICE]))});
  race_table.add_row(
      {"Metab:", std::format("{:.2f}", vm.metabolism), "",
       "nitrogen:", std::format("{}%", vm.conditions[NITROGEN]), "",
       sector_type_name(SectorType::SEC_MOUNT), std::format("{}", CHAR_MOUNT),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_MOUNT]))});
  race_table.add_row(
      {"Sexes:", std::format("{}", vm.number_sexes), "",
       "CO2:", std::format("{}%", vm.conditions[CO2]), "",
       sector_type_name(SectorType::SEC_LAND), std::format("{}", CHAR_LAND),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_LAND]))});
  race_table.add_row(
      {"Explore:", std::format("{:.0f}%", fraction_to_percent(vm.adventurism)),
       "", "hydrogen:", std::format("{}%", vm.conditions[HYDROGEN]), "",
       sector_type_name(SectorType::SEC_DESERT), std::format("{}", CHAR_DESERT),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_DESERT]))});
  race_table.add_row(
      {"Avg Int:", std::format("{}", vm.iq), "",
       "sulfur:", std::format("{}%", vm.conditions[SULFUR]), "",
       sector_type_name(SectorType::SEC_FOREST), std::format("{}", CHAR_FOREST),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_FOREST]))});
  race_table.add_row(
      {"Tech:", std::format("{:.2f}", vm.tech), "",
       "other:", std::format("{}%", vm.conditions[OTHER]), "",
       sector_type_name(SectorType::SEC_PLATED), std::format("{}", CHAR_PLATED),
       std::format("{:.0f}", fraction_to_percent(
                                 vm.sector_compat[SectorType::SEC_PLATED]))});

  std::format_to(std::back_inserter(out),
                 "{}\n\nDiscoveries:", race_table.str());
  if (vm.discoveries.crystal) out += "  Crystals";
  if (vm.discoveries.hyperdrive) out += "  Hyper-drive";
  if (vm.discoveries.laser) out += "  Combat Lasers";
  if (vm.discoveries.cew) out += "  Confined Energy Weapons";
  if (vm.discoveries.vn) out += "  Von Neumann Machines";
  if (vm.discoveries.tractor_beam) out += "  Tractor Beam";
  if (vm.discoveries.transporter) out += "  Transporter";
  if (vm.discoveries.avpm) out += "  AVPM";
  if (vm.discoveries.cloak) out += "  Cloaking";
  if (vm.discoveries.wormhole) out += "  Wormhole";
  out += "\n";
  return out;
}

std::string render_json_profile_self(const ProfileSelfViewModel& vm) {
  return render_json_envelope("profile_self", vm);
}

std::string render_for_mode(UiMode mode, const ProfileSelfViewModel& vm) {
  return mode == UiMode::JSON ? render_json_profile_self(vm)
                              : render_profile_self(vm);
}

std::string render_profile_other(const ProfileOtherViewModel& vm) {
  std::string out = std::format("------ Race report on {} ({}) ------\n",
                                vm.race_name, vm.player);
  if (vm.show_deity_status) {
    out += "*** Deity Status ***\n";
  }
  std::format_to(std::back_inserter(out),
                 "Personal: {}\n"
                 "%%Know:  {}%\n",
                 vm.personal_info, vm.know);
  if (vm.is_metamorph) {
    std::format_to(std::back_inserter(out), "{}\t  Planet Conditions\n",
                   *vm.is_metamorph ? "Metamorphic Race" : "Normal Race\t");
  } else {
    out += "Unknown Race\t\t  Planet Conditions\n";
  }
  std::format_to(std::back_inserter(out),
                 "Fert:    {}\t\t  Temp:\t{}\n"
                 "Rate:    {}\t\t  methane  {}%\t\tRanges:\n"
                 "Mass:    {}\t\t  oxygen   {}%\t\t  guns:   {}\n"
                 "Fight:   {}\t\t  helium   {}%\t\t  space:  {}\n"
                 "Metab:   {}\t\t  nitrogen {}%\t\t  ground: {}\n"
                 "Sexes:   {}\t\t  CO2      {}%\n"
                 "Explore: {}%\t\t  hydrogen {}%\n"
                 "Avg Int: {}\t\t  sulfer   {}%\n"
                 "Tech:    {}\t\t  other    {}%\t\tMorale:   {}\n"
                 "Sector type preference : {}\n",
                 vm.fertilize, vm.temp, vm.birthrate, vm.methane, vm.mass,
                 vm.oxygen, vm.gun_range, vm.fighters, vm.helium,
                 vm.space_tele_range, vm.metabolism, vm.nitrogen,
                 vm.ground_tele_range, vm.number_sexes, vm.co2, vm.adventurism,
                 vm.hydrogen, vm.iq, vm.sulfur, vm.tech, vm.other, vm.morale,
                 vm.best_sector);
  return out;
}

std::string render_json_profile_other(const ProfileOtherViewModel& vm) {
  return render_json_envelope("profile_other", vm);
}

std::string render_for_mode(UiMode mode, const ProfileOtherViewModel& vm) {
  return mode == UiMode::JSON ? render_json_profile_other(vm)
                              : render_profile_other(vm);
}

}  // namespace GB::presentation
