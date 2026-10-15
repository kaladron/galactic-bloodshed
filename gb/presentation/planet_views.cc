// SPDX-License-Identifier: Apache-2.0

/// \file planet_views.cc
/// \brief Implementation of planetary and stellar view model renderers.

module gb.presentation.planet_views;

import tabulate;
import std;

namespace GB::presentation {

namespace {

constexpr std::string_view terrain_ansi_color(char glyph) noexcept {
  switch (glyph) {
    case CHAR_SEA:
      return "\x1b[34m";
    case CHAR_LAND:
    case CHAR_FOREST:
      return "\x1b[32m";
    case CHAR_DESERT:
    case CHAR_PLATED:
      return "\x1b[33m";
    case CHAR_MOUNT:
      return "\x1b[35m";
    case CHAR_GAS:
    case CHAR_CRYSTAL:
      return "\x1b[36m";
    case CHAR_WASTED:
      return "\x1b[31m";
    default:
      return {};
  }
}

}  // namespace

std::string render_ascii_planet_map(const PlanetMapViewModel& vm) {
  std::string out;
  std::format_to(std::back_inserter(out), "     {}\n", vm.planet_name);

  if (vm.dimensions.x >= 10) {
    out += "   ";
    for (const int x : std::views::iota(0, vm.dimensions.x)) {
      out.push_back(static_cast<char>('0' + ((x / 10) % 10)));
    }
    out.push_back('\n');
  }

  out += "   ";
  for (const int x : std::views::iota(0, vm.dimensions.x)) {
    out.push_back(static_cast<char>('0' + (x % 10)));
  }
  out.push_back('\n');

  if (vm.dimensions.x > 0 && vm.dimensions.y > 0) {
    bool in_inverse = false;
    for (const auto& cell : vm.sectors) {
      if (cell.coords.x == 0) {
        std::format_to(std::back_inserter(out), "{:02d} ", cell.coords.y);
      }
      if (cell.inverse && !in_inverse) {
        out += "\x1b[7m";
        in_inverse = true;
      } else if (!cell.inverse && in_inverse) {
        out += "\x1b[27m";
        in_inverse = false;
      }
      const std::string_view color =
          vm.use_color ? terrain_ansi_color(cell.glyph) : std::string_view{};
      if (!color.empty()) {
        out += color;
      }
      out.push_back(cell.glyph);
      if (!color.empty()) {
        out += "\x1b[39m";
      }
      if (cell.coords.x + 1 == vm.dimensions.x) {
        if (in_inverse) {
          out += "\x1b[27m";
          in_inverse = false;
        }
        out.push_back('\n');
      }
    }
  }
  out.push_back('\n');

  std::format_to(
      std::back_inserter(out),
      "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", vm.planet_type_name,
      vm.is_metamorph ? "covered" : "owned", vm.sectors_owned);
  if (vm.aliens_unknown) {
    out += "???";
  } else if (vm.aliens.empty()) {
    out += "(none)";
  } else {
    for (const auto& alien : vm.aliens) {
      std::format_to(std::back_inserter(out), "{}{}", alien.at_war ? '*' : ' ',
                     alien.player);
    }
  }
  out.push_back('\n');

  std::string compat_str = std::format("{:.2f}%", vm.compatibility);
  if (vm.toxicity > 50) {
    std::format_to(std::back_inserter(compat_str), " ({}% TOXIC)", vm.toxicity);
  }

  tabulate::Table stats_table;
  stats_table.format().hide_border().column_separator(" ");
  stats_table.column(0).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(1).format().width(10);
  stats_table.column(2).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(3).format().width(26);

  stats_table.add_row({"Guns :", std::format("{}", vm.guns),
                       "Mob Points :", std::format("{}", vm.mob_points)});
  stats_table.add_row(
      {"Mobilization :", std::format("{} ({})", vm.comread, vm.mob_set),
       "Compatibility :", compat_str});
  stats_table.add_row(
      {"Resource stockpile :", std::format("{}", vm.resource_stockpile),
       "Fuel stockpile :", std::format("{}", vm.fuel_stockpile)});
  stats_table.add_row(
      {"Destruct cap :", std::format("{}", vm.destruct_cap),
       vm.is_metamorph ? "Tons of biomass :" : "Total Population :",
       std::format("{} ({}/{})", vm.player_popn, vm.total_popn,
                   vm.effective_maxpopn)});
  stats_table.add_row(
      {"Crystals :", std::format("{}", vm.crystals), "Ground forces :",
       std::format("{} ({})", vm.player_troops, vm.total_troops)});

  out += stats_table.str();
  out.push_back('\n');

  std::format_to(std::back_inserter(out),
                 "{} Total Resource Deposits     Tax rate {}%  New {}%\n"
                 "Estimated Production Next Update : {:.2f}\n",
                 vm.total_resources, vm.tax, vm.newtax, vm.est_production);

  if (vm.slaved_to) {
    std::format_to(std::back_inserter(out), "      ENSLAVED to player {};\n",
                   *vm.slaved_to);
  }
  if (vm.primary_unstable) {
    out += "WARNING! This planet's primary is unstable.\n";
  }
  return out;
}

std::string render_json_planet_map(const PlanetMapViewModel& vm) {
  return render_json_envelope("map", vm);
}

std::string render_for_mode(UiMode mode, const PlanetMapViewModel& vm) {
  return mode == UiMode::JSON ? render_json_planet_map(vm)
                              : render_ascii_planet_map(vm);
}

std::string render_survey_sectors(const SurveySectorViewModel& vm) {
  tabulate::Table table;
  table.format().hide_border().column_separator(" ");

  table.column(0).format().width(6);
  table.column(1).format().width(9);
  table.column(2).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(5).font_align(tabulate::FontAlign::right);

  table.add_row({"x,y", "cond/type", "owner", "eff", "mob", "frt", "res", "mil",
                 "popn", "^popn", "xtals"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  for (const auto& row : vm.sectors) {
    if (row.desshow_char == CHAR_CLOAKED) {
      table.add_row({std::format("{}", row.coords), "?  (    ?)", "", "", "",
                     "", "", "", "", "", ""});
    } else {
      std::string cond_type =
          std::format(" {}   {}", row.sect_char, row.type_char);
      std::string crystals = row.crystals ? "yes" : "";
      table.add_row({std::format("{}", row.coords), cond_type,
                     std::format("{}", row.owner), std::format("{}", row.eff),
                     std::format("{}", row.mobilization),
                     std::format("{}", row.fert),
                     std::format("{}", row.resource),
                     std::format("{}", row.troops), std::format("{}", row.popn),
                     std::format("{}", row.maxpopn), crystals});
    }
  }

  std::string out = table.str();
  out.push_back('\n');
  return out;
}

std::string render_json_survey_sectors(const SurveySectorViewModel& vm) {
  return render_json_envelope("survey_sectors", vm);
}

std::string render_for_mode(UiMode mode, const SurveySectorViewModel& vm) {
  return mode == UiMode::JSON ? render_json_survey_sectors(vm)
                              : render_survey_sectors(vm);
}

namespace {

constexpr std::array<std::string_view, 12> TOX_DESCRIPTIONS = {
    "Stage 0, mild",
    "Stage 1, mild",
    "Stage 2, semi-mild",
    "Stage 3, semi-semi mild",
    "Stage 4, ecologically unsound",
    "Stage 5: ecologically unsound",
    "Stage 6: below birth threshold",
    "Stage 7: ecologically unstable--below birth threshold",
    "Stage 8: ecologically poisonous --below birth threshold",
    "Stage 9: WARNING: nearing 100% toxicity",
    "Stage 10: WARNING: COMPLETELY TOXIC!!!",
    "???"};

std::string_view toxicity_description(Percentage toxicity) {
  const int index = std::clamp(toxicity.value() / 10, 0, 11);
  return TOX_DESCRIPTIONS[index];
}

std::string_view stability_label(int pct) {
  if (pct < 20) return "stable";
  if (pct < 40) return "unstable";
  if (pct < 60) return "dangerous";
  if (pct < 100) return "WARNING! Nova iminent!";
  return "undergoing nova";
}

}  // namespace

std::string
render_survey_planet_overview(const SurveyPlanetOverviewViewModel& vm) {
  const std::string_view pop_label = vm.is_metamorph ? "biomass" : "popltn";
  std::string out = std::format(
      "{}:\n"
      "gravity   x,y absolute     x,y relative to {}\n"
      "{:7.2f}   {:7.1f},{:7.1f}   {:8.1f},{:8.1f}\n"
      "======== Planetary conditions: ========\n"
      "atmosphere concentrations:\n"
      "     methane {:02d}%({:02d}%)     oxygen {:02d}%({:02d}%)\n"
      "         CO2 {:02d}%({:02d}%)   hydrogen {:02d}%({:02d}%)      "
      "temperature: {:3d} ({:3d})\n"
      "    nitrogen {:02d}%({:02d}%)     sulfur {:02d}%({:02d}%)           "
      "normal: {:3d}\n"
      "      helium {:02d}%({:02d}%)      other {:02d}%({:02d}%)\n"
      "                     Toxicity: {}% ({})\n"
      "Total planetary compatibility: {:.2f}%\n"
      "{:>29}: {}\n{:>29}: {}\n{:>29}: {}\n"
      "{:>29}: {}\n"
      "fuel_stock  resource_stock dest_pot.   {}    ^{}\n"
      "{:10}  {:14} {:9}  {:7}{:11}\n",
      vm.planet_name, vm.star_name, vm.gravity, vm.absolute_coords.x,
      vm.absolute_coords.y, vm.system_coords.x, vm.system_coords.y,
      vm.planet_conditions.methane, vm.race_conditions.methane,
      vm.planet_conditions.oxygen, vm.race_conditions.oxygen,
      vm.planet_conditions.co2, vm.race_conditions.co2,
      vm.planet_conditions.hydrogen, vm.race_conditions.hydrogen,
      vm.planet_temp, vm.race_temp, vm.planet_conditions.nitrogen,
      vm.race_conditions.nitrogen, vm.planet_conditions.sulfur,
      vm.race_conditions.sulfur, vm.normal_temp, vm.planet_conditions.helium,
      vm.race_conditions.helium, vm.planet_conditions.other,
      vm.race_conditions.other, vm.toxicity, toxicity_description(vm.toxicity),
      vm.compatibility, "Average fertility", vm.avg_fertility,
      "Average resource", vm.avg_resource, "Crystal sectors",
      vm.crystal_sectors, "Total resource deposits", vm.total_resources,
      pop_label, pop_label, vm.fuel_stock, vm.resource_stock, vm.destruct_stock,
      vm.popn, vm.maxpopn);
  if (vm.slaved_to) {
    std::format_to(std::back_inserter(out),
                   "This planet ENSLAVED to player {}!\n", *vm.slaved_to);
  }
  return out;
}

std::string
render_json_survey_planet_overview(const SurveyPlanetOverviewViewModel& vm) {
  return render_json_envelope("survey_planet", vm);
}

std::string render_for_mode(UiMode mode,
                            const SurveyPlanetOverviewViewModel& vm) {
  return mode == UiMode::JSON ? render_json_survey_planet_overview(vm)
                              : render_survey_planet_overview(vm);
}

std::string render_survey_star(const SurveyStarViewModel& vm) {
  std::string out =
      std::format("Star {}\nlocn: {}\n", vm.star_name, vm.coordinates);

  if (vm.is_god) {
    for (const auto& name : vm.planet_names) {
      std::format_to(std::back_inserter(out), " \"{}\"\n", name);
    }
  }

  std::format_to(std::back_inserter(out),
                 "Gravity: {:.2f}\tInstability: ", vm.gravity);

  if (vm.stability) {
    std::format_to(std::back_inserter(out), "{}% ({})\n", *vm.stability,
                   stability_label(*vm.stability));
  } else {
    out += "(cannot determine)\n";
  }

  std::format_to(std::back_inserter(out),
                 "temperature class (1->10) {}\n{} planets are ",
                 vm.temperature_class, vm.planet_names.size());
  for (const auto& name : vm.planet_names) {
    std::format_to(std::back_inserter(out), "{} ", name);
  }
  out.push_back('\n');
  return out;
}

std::string render_json_survey_star(const SurveyStarViewModel& vm) {
  return render_json_envelope("survey_star", vm);
}

std::string render_for_mode(UiMode mode, const SurveyStarViewModel& vm) {
  return mode == UiMode::JSON ? render_json_survey_star(vm)
                              : render_survey_star(vm);
}

std::string render_survey_universe(const SurveyUniverseViewModel& vm) {
  return std::format("{}\n", vm.message);
}

std::string render_json_survey_universe(const SurveyUniverseViewModel& vm) {
  return render_json_envelope("survey_universe", vm);
}

std::string render_for_mode(UiMode mode, const SurveyUniverseViewModel& vm) {
  return mode == UiMode::JSON ? render_json_survey_universe(vm)
                              : render_survey_universe(vm);
}

namespace {

constexpr int ORBIT_CANVAS_WIDTH = 60;
constexpr int ORBIT_CANVAS_HEIGHT = 20;
constexpr double ORBIT_MAX_COORD = 200.0;

struct OrbitCanvasCell {
  char ch{' '};
  bool inverse{false};
};

using OrbitCanvasGrid =
    std::array<std::array<OrbitCanvasCell, ORBIT_CANVAS_WIDTH>,
               ORBIT_CANVAS_HEIGHT>;

void plot_orbit_symbol(OrbitCanvasGrid& grid, SystemCoordinates coords,
                       char symbol, bool inverse) {
  const int col = std::clamp(
      static_cast<int>(coords.x * (ORBIT_CANVAS_WIDTH - 1) / ORBIT_MAX_COORD),
      0, ORBIT_CANVAS_WIDTH - 1);
  const int row = std::clamp(
      static_cast<int>(coords.y * (ORBIT_CANVAS_HEIGHT - 1) / ORBIT_MAX_COORD),
      0, ORBIT_CANVAS_HEIGHT - 1);
  grid[row][col] = OrbitCanvasCell{.ch = symbol, .inverse = inverse};
}

void append_standout(std::string& out, std::string_view text, bool inverse) {
  if (inverse) {
    std::format_to(std::back_inserter(out), "\x1b[7m{}\x1b[27m", text);
  } else {
    out += text;
  }
}

void append_standout_char(std::string& out, char ch, bool inverse) {
  if (inverse) {
    std::format_to(std::back_inserter(out), "\x1b[7m{}\x1b[27m", ch);
  } else {
    out.push_back(ch);
  }
}

}  // namespace

std::string render_ascii_orbit_map(const OrbitViewModel& vm) {
  OrbitCanvasGrid grid{};
  for (const auto& star : vm.stars) {
    plot_orbit_symbol(grid, star.coords, '*',
                      vm.inverse && star.symbol_standout);
  }
  for (const auto& planet : vm.planets) {
    plot_orbit_symbol(grid, planet.coords, planet.symbol,
                      vm.inverse && planet.symbol_standout);
  }
  for (const auto& ship : vm.ships) {
    plot_orbit_symbol(grid, ship.coords, ship.symbol,
                      vm.inverse && ship.standout);
  }

  std::string out;
  for (const auto& row : grid) {
    bool in_inverse = false;
    for (const auto& cell : row) {
      if (cell.inverse && !in_inverse) {
        out += "\x1b[7m";
        in_inverse = true;
      } else if (!cell.inverse && in_inverse) {
        out += "\x1b[27m";
        in_inverse = false;
      }
      out.push_back(cell.ch);
    }
    if (in_inverse) {
      out += "\x1b[27m";
    }
    out.push_back('\n');
  }

  for (const auto& star : vm.stars) {
    if (!star.label.empty()) {
      out += "  ";
      append_standout_char(out, '*', vm.inverse && star.symbol_standout);
      out.push_back(' ');
      append_standout(out, star.label, vm.inverse && star.label_standout);
      out.push_back('\n');
    }
  }
  for (const auto& planet : vm.planets) {
    if (!planet.label.empty()) {
      out += "  ";
      append_standout_char(out, planet.symbol,
                           vm.inverse && planet.symbol_standout);
      out.push_back(' ');
      append_standout(out, planet.label, vm.inverse && planet.label_standout);
      if (planet.compatibility) {
        std::format_to(std::back_inserter(out), "({})", *planet.compatibility);
      }
      out.push_back('\n');
    }
  }
  for (const auto& ship : vm.ships) {
    out += "  ";
    const std::string ship_tag =
        std::format("{} #{}", ship.symbol, ship.shipno);
    append_standout(out, ship_tag, vm.inverse && ship.standout);
    if (ship.symbol == 'M') {
      std::format_to(std::back_inserter(out), " (aim: {})",
                     ship.mirror_heading);
    }
    out.push_back('\n');
  }

  return out;
}

std::string render_json_orbit_map(const OrbitViewModel& vm) {
  return render_json_envelope("orbit", vm);
}

std::string render_for_mode(UiMode mode, const OrbitViewModel& vm) {
  return mode == UiMode::JSON ? render_json_orbit_map(vm)
                              : render_ascii_orbit_map(vm);
}

std::string render_explore(const ExploreViewModel& vm) {
  std::string out =
      std::format("         ========== Exploration Report ==========\n"
                  " Global action points : [{:2}]\n",
                  vm.global_ap);

  for (const auto& star : vm.stars) {
    if (star.stability) {
      std::format_to(std::back_inserter(out), "\n{} ({:2})[{:2}]\n",
                     star.star_name, *star.stability, star.ap);
    } else {
      std::format_to(std::back_inserter(out), "\n{} (/?/?)[{:2}]\n",
                     star.star_name, star.ap);
    }

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");
    table.column(0).format().width(3).font_align(tabulate::FontAlign::right);
    table.column(1).format().width(15);
    table.column(2).format().width(30);
    table.column(3).format().width(12);
    table.column(4).format().width(6).font_align(tabulate::FontAlign::right);

    table.add_row({"#", "Planet", "Attributes", "Type", "Compat"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (const auto& planet : star.planets) {
      std::string attrs;
      std::string type_col;
      std::string compat_col;
      if (planet.explored) {
        attrs += "Ex ";
        if (planet.autorep) attrs += "Rep ";
        if (planet.inhabited) attrs += "Inhab ";
        if (planet.enslaved) attrs += "SLAVED ";
        for (const player_t owner : planet.alien_owners) {
          std::format_to(std::back_inserter(attrs), "{} ", owner);
        }
        if (planet.toxic) attrs += "TOXIC ";

        type_col = planet.type_name;
        compat_col = std::format("{:.0f}%", planet.compatibility);
      } else {
        attrs = "No Data";
      }

      table.add_row({std::format("{}", planet.planet_order), planet.planet_name,
                     attrs, type_col, compat_col});
    }

    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
  return out;
}

std::string render_json_explore(const ExploreViewModel& vm) {
  return render_json_envelope("explore", vm);
}

std::string render_for_mode(UiMode mode, const ExploreViewModel& vm) {
  return mode == UiMode::JSON ? render_json_explore(vm) : render_explore(vm);
}

std::string render_analysis(const AnalysisViewModel& vm) {
  std::string out;

  for (const auto& report : vm.planets) {
    std::format_to(std::back_inserter(out), "\nAnalysis of /{}/{}:\n{} {} {}{}",
                   report.star_name, report.planet_name,
                   report.is_top_five ? "Highest" : "Lowest", report.care_count,
                   report.sector_type_name ? *report.sector_type_name
                                           : "of all",
                   report.filter_description);

    auto append_top_line = [&](std::string_view name,
                               std::span<const AnalysisSectorEntry> entries) {
      std::format_to(std::back_inserter(out), "{:>8}:", name);
      for (const auto& entry : entries) {
        std::format_to(std::back_inserter(out), "{:>5}{}({:>2},{:>2})",
                       entry.value, entry.sector_char, entry.coords.x,
                       entry.coords.y);
      }
      out.push_back('\n');
    };

    append_top_line("Troops", report.troops);
    append_top_line("Res", report.res);
    append_top_line("Eff", report.eff);
    append_top_line("Frt", report.frt);
    append_top_line("Mob", report.mob);
    append_top_line("Popn", report.popn);
    append_top_line("^Popn", report.m_popn);
    out.push_back('\n');

    tabulate::Table table;
    table.format().hide_border().column_separator("  ");

    std::vector<std::string> table_header = {"Pl",    "sec",   "popn", "troops",
                                             "a.eff", "a.mob", "res",  "x"};
    for (const SectorType st : all_sector_types) {
      table_header.emplace_back(1, get_sector_char(st));
    }
    table.add_row(
        tabulate::Table::Row_t(table_header.begin(), table_header.end()));
    table[0].format().font_style({tabulate::FontStyle::bold});

    for (const auto& r : report.rows) {
      std::vector<std::string> row = {r.label,
                                      std::format("{}", r.sectors),
                                      std::format("{}", r.popn),
                                      std::format("{}", r.troops),
                                      std::format("{:.1f}", r.avg_eff),
                                      std::format("{:.1f}", r.avg_mob),
                                      std::format("{}", r.res),
                                      std::format("{}", r.crystals)};
      for (const int count : r.by_type) {
        row.push_back(std::format("{}", count));
      }
      table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
    }

    for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
      table.column(8 + i).format().width(4).font_align(
          tabulate::FontAlign::right);
    }

    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }

  return out;
}

std::string render_json_analysis(const AnalysisViewModel& vm) {
  return render_json_envelope("analysis", vm);
}

std::string render_for_mode(UiMode mode, const AnalysisViewModel& vm) {
  return mode == UiMode::JSON ? render_json_analysis(vm) : render_analysis(vm);
}

std::string render_mobilize_status(const MobilizeStatusViewModel& vm) {
  return std::format("Current mobilization: {}    Quota: {}\n",
                     vm.current_mobilization, vm.quota);
}

std::string render_json_mobilize_status(const MobilizeStatusViewModel& vm) {
  return render_json_envelope("mobilize_status", vm);
}

std::string render_for_mode(UiMode mode, const MobilizeStatusViewModel& vm) {
  return mode == UiMode::JSON ? render_json_mobilize_status(vm)
                              : render_mobilize_status(vm);
}

std::string render_mobilize_set_result(const MobilizeSetResult&) {
  return "";
}

std::string render_json_mobilize_set_result(const MobilizeSetResult& vm) {
  return render_json_envelope("mobilize", vm);
}

std::string render_for_mode(UiMode mode, const MobilizeSetResult& vm) {
  return mode == UiMode::JSON ? render_json_mobilize_set_result(vm)
                              : render_mobilize_set_result(vm);
}

std::string render_tax_status(const TaxStatusViewModel& vm) {
  return std::format("Current tax rate: {}%    Target: {}%\n", vm.current_rate,
                     vm.target_rate);
}

std::string render_json_tax_status(const TaxStatusViewModel& vm) {
  return render_json_envelope("tax_status", vm);
}

std::string render_for_mode(UiMode mode, const TaxStatusViewModel& vm) {
  return mode == UiMode::JSON ? render_json_tax_status(vm)
                              : render_tax_status(vm);
}

std::string render_tax_set_result(const TaxSetResult&) {
  return "Set.\n";
}

std::string render_json_tax_set_result(const TaxSetResult& vm) {
  return render_json_envelope("tax", vm);
}

std::string render_for_mode(UiMode mode, const TaxSetResult& vm) {
  return mode == UiMode::JSON ? render_json_tax_set_result(vm)
                              : render_tax_set_result(vm);
}

std::string render_toxicity_result(const ToxicityResult& vm) {
  return std::format(" New threshold is: {}\n", vm.threshold);
}

std::string render_json_toxicity_result(const ToxicityResult& vm) {
  return render_json_envelope("toxicity", vm);
}

std::string render_for_mode(UiMode mode, const ToxicityResult& vm) {
  return mode == UiMode::JSON ? render_json_toxicity_result(vm)
                              : render_toxicity_result(vm);
}

std::string render_allocate_result(const AllocateResult&) {
  return "Allocated\n";
}

std::string render_json_allocate_result(const AllocateResult& vm) {
  return render_json_envelope("allocate", vm);
}

std::string render_for_mode(UiMode mode, const AllocateResult& vm) {
  return mode == UiMode::JSON ? render_json_allocate_result(vm)
                              : render_allocate_result(vm);
}

}  // namespace GB::presentation
