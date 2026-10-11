// SPDX-License-Identifier: Apache-2.0

/// \file empire_views.cc
/// \brief ASCII and Glaze JSON renderers for empire and diplomatic view models.

module gb.presentation.empire_views;

import gb.entities;
import gb.services;
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

}  // namespace GB::presentation
