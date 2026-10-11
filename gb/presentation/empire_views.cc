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

}  // namespace GB::presentation
