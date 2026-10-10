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

}  // namespace GB::presentation
