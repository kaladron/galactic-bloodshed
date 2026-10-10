// SPDX-License-Identifier: Apache-2.0

/// \file planet_views.cc
/// \brief Implementation of planetary and stellar view model renderers.

module gb.presentation.planet_views;

import tabulate;
import std;

namespace GB::presentation {

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
      out.push_back(cell.glyph);
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

}  // namespace GB::presentation
