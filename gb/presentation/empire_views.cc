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

std::string render_block(const BlockViewModel& vm) {
  std::string out;

  if (vm.player_report) {
    const auto& pr = *vm.player_report;
    auto append_block_list = [&](std::string_view prefix,
                                 std::span<const player_t> blocks) {
      std::format_to(std::back_inserter(out), "Race #{} [{}] {}", pr.player,
                     pr.race_name, prefix);
      if (blocks.empty()) {
        out += "no blocks\n";
        return;
      }
      for (std::size_t idx = 0; idx < blocks.size(); ++idx) {
        std::format_to(std::back_inserter(out), "{}{}", idx > 0 ? ", " : " ",
                       blocks[idx]);
      }
      out.push_back('\n');
    };

    append_block_list("is a member of ", pr.member_blocks);
    append_block_list("has been invited to join ", pr.invited_blocks);
    append_block_list("has pledged ", pr.pledged_blocks);
  }

  if (vm.detail_report) {
    const auto& dr = *vm.detail_report;
    std::format_to(std::back_inserter(out),
                   "         ========== {} Power Report ==========\n"
                   "                 {:<64.64}\n",
                   dr.block_name, dr.motto);

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");
    table.add_row({"#", "Name", "troops", "pop", "money", "ship", "plan", "res",
                   "fuel", "dest", "know"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (const auto& m : dr.members) {
      table.add_row({std::format("{}", m.player), m.name, m.troops, m.pop,
                     m.money, m.ships, m.planets, m.resource, m.fuel,
                     m.destruct, std::format("{}%", m.know)});
    }
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }

  if (vm.list_report) {
    const auto& lr = *vm.list_report;
    std::format_to(
        std::back_inserter(out),
        "         ========== Global Alliance Block Report ==========\n"
        "                 (updated: {:<24.24})\n",
        lr.update_time);

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");
    table.add_row({"#", "Name", "memb", "money", "popn", "ship", "sys", "res",
                   "fuel", "dest", "VPs", "know"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (const auto& b : lr.blocks) {
      table.add_row({std::format("{}", b.player), b.name,
                     std::format("{}", b.members), b.money, b.popn, b.ships,
                     b.systems, b.resource, b.fuel, b.destruct, b.vps,
                     std::format("{}%", b.know)});
    }
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }

  return out;
}

std::string render_json_block(const BlockViewModel& vm) {
  return render_json_envelope("block", vm);
}

std::string render_for_mode(UiMode mode, const BlockViewModel& vm) {
  return mode == UiMode::JSON ? render_json_block(vm) : render_block(vm);
}

std::string render_colonies(const ColoniesViewModel& vm) {
  std::string out = "          ========== Colonization Report ==========\n"
                    "  Planet     gov sec tech    popn  x   res  "
                    "des  fuel  tax  cmpt/tox mob  Aliens\n";

  for (const auto& err : vm.errors) {
    out += err;
  }

  for (const auto& row : vm.rows) {
    std::format_to(
        std::back_inserter(out),
        " {:c} {:4.4}/{:<4.4}{:c}{:4d}{:3d}{:5d}{:8d}{:3d}{:6d}{:5d}{:6d} "
        "{:3d}/{:<3d}{:3.0f}/{:<3d}{:3d}/{:<3d}",
        row.type_symbol, row.star_name, row.planet_name,
        row.autorep ? '*' : ' ', row.governor, row.sectors_owned,
        row.tech_invest, row.popn, row.crystals, row.resource, row.destruct,
        row.fuel, row.tax, row.newtax, row.compatibility, row.toxicity,
        row.comread, row.mob_set);
    for (const player_t alien : row.alien_owners) {
      std::format_to(std::back_inserter(out), " {}", alien);
    }
    out.push_back('\n');
  }
  out.push_back('\n');
  return out;
}

std::string render_json_colonies(const ColoniesViewModel& vm) {
  return render_json_envelope("colonies", vm);
}

std::string render_for_mode(UiMode mode, const ColoniesViewModel& vm) {
  return mode == UiMode::JSON ? render_json_colonies(vm) : render_colonies(vm);
}

std::string render_power(const PowerViewModel& vm) {
  std::string out =
      "         ========== Galactic Bloodshed Power Report ==========\n";

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(4);
  table.column(2).format().width(2);
  table.column(3).format().width(15);
  table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(13).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(14).format().width(5).font_align(tabulate::FontAlign::right);

  table.add_row({vm.show_rank_header ? "rank" : "", "#", "", "Name", "VP",
                 "mil", "civ", "cash", "ship", "pl", "res", "fuel", "dest",
                 "morl", vm.is_god ? "VNs" : "know"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& row : vm.rows) {
    table.add_row({row.rank != 0 ? std::format("{}", row.rank) : "",
                   std::format("[{:2d}]", row.player), row.alliance, row.name,
                   row.vp, row.mil, row.civ, row.cash, row.ships, row.planets,
                   row.resource, row.fuel, row.destruct, row.morale,
                   row.know_or_vns});
  }

  std::format_to(std::back_inserter(out), "{}\n", table.str());
  return out;
}

std::string render_json_power(const PowerViewModel& vm) {
  return render_json_envelope("power", vm);
}

std::string render_for_mode(UiMode mode, const PowerViewModel& vm) {
  return mode == UiMode::JSON ? render_json_power(vm) : render_power(vm);
}

std::string render_tech_status(const TechStatusViewModel& vm) {
  std::string out = "             ========== Technology Report ==========\n\n";

  for (const auto& err : vm.errors) {
    out += err;
  }

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(16);
  table.column(1).format().width(10).font_align(tabulate::FontAlign::right);
  table.column(2).format().width(10).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::right);

  table.add_row({"Planet", "popn", "invest", "gain", "^gain"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& row : vm.rows) {
    table.add_row({row.location, std::format("{}", row.popn),
                   std::format("{}", row.tech_invest),
                   std::format("{:.3f}", row.gain),
                   std::format("{:.3f}", row.max_gain)});
  }

  std::format_to(std::back_inserter(out), "{}\n", table.str());

  if (!vm.total_popn) {
    out += "       Total Popn:  unknown\n";
  } else {
    std::format_to(std::back_inserter(out), "       Total Popn:  {:7}\n",
                   *vm.total_popn);
  }
  std::format_to(std::back_inserter(out), "Tech: {:31}{:8.3f}{:8.3f}\n",
                 vm.total_invest, vm.total_gain, vm.total_max_gain);
  return out;
}

std::string render_json_tech_status(const TechStatusViewModel& vm) {
  return render_json_envelope("tech_status", vm);
}

std::string render_for_mode(UiMode mode, const TechStatusViewModel& vm) {
  return mode == UiMode::JSON ? render_json_tech_status(vm)
                              : render_tech_status(vm);
}

std::string render_examine(const ExamineViewModel& vm) {
  std::string out;
  if (!vm.description.empty()) {
    std::format_to(std::back_inserter(out), "\n{}", vm.description);
    if (!vm.description.ends_with('\n')) {
      out.push_back('\n');
    }
  }
  if (vm.has_switch) {
    out += "This device has an on/off switch that can be set with order.\n";
  }
  if (vm.irradiated) {
    out += "This device has been irradiated;\n"
           "Its crew is dying and it cannot move for the time being.\n";
  }
  return out;
}

std::string render_json_examine(const ExamineViewModel& vm) {
  return render_json_envelope("examine", vm);
}

std::string render_for_mode(UiMode mode, const ExamineViewModel& vm) {
  return mode == UiMode::JSON ? render_json_examine(vm) : render_examine(vm);
}

std::string render_dump(const DumpViewModel& vm) {
  std::string out;
  for (const auto& err : vm.errors) {
    out += err;
  }
  out += "Exploration Data transferred.\n";
  return out;
}

std::string render_json_dump(const DumpViewModel& vm) {
  return render_json_envelope("dump", vm);
}

std::string render_for_mode(UiMode mode, const DumpViewModel& vm) {
  return mode == UiMode::JSON ? render_json_dump(vm) : render_dump(vm);
}

std::string render_time(const TimeViewModel& vm) {
  return std::format("{}{}{}"
                     "Current time    : {}\n",
                     vm.start_line, vm.update_line, vm.segment_line,
                     vm.current_time);
}

std::string render_json_time(const TimeViewModel& vm) {
  return render_json_envelope("time", vm);
}

std::string render_for_mode(UiMode mode, const TimeViewModel& vm) {
  return mode == UiMode::JSON ? render_json_time(vm) : render_time(vm);
}

std::string render_schedule(const ScheduleViewModel& vm) {
  return std::format("{} minute update intervals\n"
                     "{} movement segments per update\n"
                     "Current time    : {}\n"
                     "Next Segment {:2d} : {}\n"
                     "Next Update {:3d} : {}\n",
                     vm.update_time_minutes, vm.segments, vm.current_time,
                     vm.next_segment_number, vm.next_segment_time,
                     vm.next_update_number, vm.next_update_time);
}

std::string render_json_schedule(const ScheduleViewModel& vm) {
  return render_json_envelope("schedule", vm);
}

std::string render_for_mode(UiMode mode, const ScheduleViewModel& vm) {
  return mode == UiMode::JSON ? render_json_schedule(vm) : render_schedule(vm);
}

std::string render_victory(const VictoryViewModel& vm) {
  std::string out = "----==== PLAYER RANKINGS ====----\n";

  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(3).format().width(15);

  tabulate::Table::Row_t header = {"No.", "", "Race", "Name"};
  if (vm.is_god) {
    table.column(4).format().width(6).font_align(tabulate::FontAlign::right);
    table.column(5).format().width(6).font_align(tabulate::FontAlign::right);
    table.column(6).format().width(3).font_align(tabulate::FontAlign::right);
    table.column(7).format().width(10);
    table.column(8).format().width(10);
    header.insert(header.end(),
                  {"Score", "Tech", "IQ", "Password", "Gov Pass"});
  }
  table.add_row(header);
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& r : vm.rows) {
    tabulate::Table::Row_t row = {std::format("{}", r.rank),
                                  std::format("{}", r.is_metamorph ? 'M' : ' '),
                                  std::format("[{}]", r.player),
                                  std::format("{:.15}", r.name)};
    if (vm.is_god) {
      row.insert(row.end(),
                 {std::format("{}", r.raw_score), std::format("{:.2f}", r.tech),
                  std::format("{}", r.iq), r.password, r.gov_password});
    }
    table.add_row(row);
  }

  std::format_to(std::back_inserter(out), "{}\n", table.str());
  return out;
}

std::string render_json_victory(const VictoryViewModel& vm) {
  return render_json_envelope("victory", vm);
}

std::string render_for_mode(UiMode mode, const VictoryViewModel& vm) {
  return mode == UiMode::JSON ? render_json_victory(vm) : render_victory(vm);
}

std::string render_prompt(const PromptViewModel& vm) {
  return std::format(" ( [{}] {} )\n", vm.ap, vm.path);
}

std::string render_json_prompt(const PromptViewModel& vm) {
  return render_json_envelope("prompt", vm);
}

std::string render_for_mode(UiMode mode, const PromptViewModel& vm) {
  return mode == UiMode::JSON ? render_json_prompt(vm) : render_prompt(vm);
}

std::string render_text(const TextViewModel& vm) {
  return std::format("{}\n", vm.message);
}

std::string render_json_text(const TextViewModel& vm) {
  return render_json_envelope("text", vm);
}

std::string render_for_mode(UiMode mode, const TextViewModel& vm) {
  return mode == UiMode::JSON ? render_json_text(vm) : render_text(vm);
}

std::string render_login_banner(const LoginViewModel& vm) {
  return std::format("\n{} \"{}\" [{},{}] logged on.\nYou are {}.\n",
                     vm.race_name, vm.gov_name, vm.race_id, vm.gov_id,
                     vm.invisible ? "invisible" : "visible");
}

std::string render_login_status(const LoginViewModel& vm) {
  return std::format(
      "\nLast login      : {}\n"
      "{}"
      "     Morale: {}\n",
      vm.last_login,
      vm.gov_ship
          ? std::format("Government Center #{} is active.\n", *vm.gov_ship)
          : "You have no Governmental Center.  No action points will be "
            "produced\nuntil you build one and designate a capital.\n",
      vm.morale);
}

std::string render_login(const LoginViewModel& vm) {
  return render_login_banner(vm) + render_login_status(vm);
}

std::string render_json_login(const LoginViewModel& vm) {
  return render_json_envelope("login", vm);
}

std::string render_for_mode(UiMode mode, const LoginViewModel& vm) {
  return mode == UiMode::JSON ? render_json_login(vm) : render_login(vm);
}

std::string render_commodity_market(const CommodityMarketViewModel& vm) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.column(0).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(10).font_align(tabulate::FontAlign::left);
  table.column(4).format().width(7).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(10).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(10).font_align(tabulate::FontAlign::left);

  table.add_row({"Lot", "", "Stock", "Type", "Owner", "Bidder", "Amount",
                 "Cost/Unit", "Ship", "Dest"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& row : vm.rows) {
    table.add_row({std::format("{}", row.lot), row.deliver ? "*" : "",
                   std::format("{}", row.amount), std::format("{}", row.type),
                   std::format("{}", row.owner), std::format("{}", row.bidder),
                   std::format("{}", row.bid),
                   std::format("{:.2f}", row.cost_per_unit),
                   std::format("{}", row.shipping_cost), row.destination});
  }

  return std::format("+++ Galactic Bloodshed Commodities Market +++\n\n{}\n",
                     table.str());
}

std::string render_json_commodity_market(const CommodityMarketViewModel& vm) {
  return render_json_envelope("commodity_market", vm);
}

std::string render_for_mode(UiMode mode, const CommodityMarketViewModel& vm) {
  return mode == UiMode::JSON ? render_json_commodity_market(vm)
                              : render_commodity_market(vm);
}

std::string render_bid_placed_result(const BidPlacedResult& vm) {
  return std::format(
      "There will be an additional {} charged to you for shipping costs.\n"
      "Bid accepted.\n",
      vm.shipping_cost);
}

std::string render_json_bid_placed_result(const BidPlacedResult& vm) {
  return render_json_envelope("bid", vm);
}

std::string render_for_mode(UiMode mode, const BidPlacedResult& vm) {
  return mode == UiMode::JSON ? render_json_bid_placed_result(vm)
                              : render_bid_placed_result(vm);
}

std::string render_outbid_telegram(int lot, resource_t amount, CommodType type,
                                   money_t new_bid,
                                   std::string_view bidder_name,
                                   player_t bidder) {
  return std::format(
      "The bid on lot #{} ({} {}) has been upped to {} by {} [{}].\n", lot,
      amount, type, new_bid, bidder_name, bidder);
}

std::string render_sell_result(const SellResult& vm) {
  return std::format("Lot #{} - {} units of {}.\n", vm.lot, vm.amount, vm.type);
}

std::string render_json_sell_result(const SellResult& vm) {
  return render_json_envelope("sell", vm);
}

std::string render_for_mode(UiMode mode, const SellResult& vm) {
  return mode == UiMode::JSON ? render_json_sell_result(vm)
                              : render_sell_result(vm);
}

std::string render_sell_market_news(int lot, resource_t amount, CommodType type,
                                    std::string_view seller_name,
                                    player_t seller) {
  return std::format("Lot #{} - {} units of {} for sale by {} [{}].\n", lot,
                     amount, type, seller_name, seller);
}

std::string render_treasury(const TreasuryViewModel& vm) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.column(0).format().width(12);
  table.column(1).format().width(8).font_align(tabulate::FontAlign::right);
  table.column(2).format().width(4);
  table.column(3).format().width(12);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::right);

  table.add_row({"Income", std::format("{}", vm.total_income), "", "Costs",
                 std::format("{}", vm.total_costs)});
  table[0].format().font_style({tabulate::FontStyle::bold});

  table.add_row({"  Market:", std::format("{}", vm.profit_market), "",
                 "  Market:", std::format("{}", vm.cost_market)});
  table.add_row({"  Taxes:", std::format("{}", vm.income_taxes), "",
                 "  Tech:", std::format("{}", vm.cost_tech)});
  table.add_row({"", "", "", "  Maint:", std::format("{}", vm.maintain)});

  return std::format("{}\nYou have: {}\n", table.str(), vm.money);
}

std::string render_json_treasury(const TreasuryViewModel& vm) {
  return render_json_envelope("treasury", vm);
}

std::string render_for_mode(UiMode mode, const TreasuryViewModel& vm) {
  return mode == UiMode::JSON ? render_json_treasury(vm) : render_treasury(vm);
}

GiveResult make_give_result(const Ship& ship, player_t recipient_id,
                            std::string_view recipient_name,
                            std::string_view orbit_path) {
  return GiveResult{
      .ship_number = ship.number(),
      .ship_desc = std::format("{}", ship),
      .recipient_id = recipient_id,
      .recipient_name = std::string(recipient_name),
      .orbit_path = std::string(orbit_path),
  };
}

std::string render_give_result(const GiveResult& /*vm*/) {
  return "Owner changed.\n";
}

std::string render_json_give_result(const GiveResult& vm) {
  return render_json_envelope("give", vm);
}

std::string render_for_mode(UiMode mode, const GiveResult& vm) {
  return mode == UiMode::JSON ? render_json_give_result(vm)
                              : render_give_result(vm);
}

std::string render_give_recipient_telegram(std::string_view donor_name,
                                           player_t donor_id,
                                           std::string_view ship_desc,
                                           std::string_view orbit_path) {
  return std::format("{} [{}] gave you {} at {}.\n", donor_name, donor_id,
                     ship_desc, orbit_path);
}

std::string render_give_news(std::string_view donor_name, player_t donor_id,
                             std::string_view recipient_name,
                             player_t recipient_id) {
  return std::format("{} [{}] gives {} [{}] a ship.\n", donor_name, donor_id,
                     recipient_name, recipient_id);
}

std::string render_pay_result(const PayResult& vm) {
  return std::format("{} payed to {} [{}].\n", vm.amount, vm.recipient_name,
                     vm.recipient_id);
}

std::string render_json_pay_result(const PayResult& vm) {
  return render_json_envelope("pay", vm);
}

std::string render_for_mode(UiMode mode, const PayResult& vm) {
  return mode == UiMode::JSON ? render_json_pay_result(vm)
                              : render_pay_result(vm);
}

std::string render_pay_recipient_telegram(std::string_view payer_name,
                                          player_t payer_id, money_t amount) {
  return std::format("{} [{}] payed you {}.\n", payer_name, payer_id, amount);
}

std::string render_pay_news(std::string_view payer_name, player_t payer_id,
                            std::string_view recipient_name,
                            player_t recipient_id) {
  return std::format("{} [{}] pays {} [{}].\n", payer_name, payer_id,
                     recipient_name, recipient_id);
}

}  // namespace GB::presentation
