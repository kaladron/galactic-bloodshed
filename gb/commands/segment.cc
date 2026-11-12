// SPDX-License-Identifier: Apache-2.0

/// \file segment.cc
/// \brief Trigger segment movement (deity only).

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import gb.turn;
import std;

module commands;

namespace GB::commands {

bool segment(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Syntax: @@segment [seg_num]\n";
    return false;
  }

  int seg_num = 0;
  if (argv.size() == 2) {
    auto res = parse_int<int>(argv[1]);
    if (!res || *res < 0) {
      g.out << "Invalid segment number.\n";
      return false;
    }
    seg_num = *res;
  }

  g.session_registry.flush_all();
  do_segment(g.entity_manager, g.session_registry, 1, seg_num);
  g.present(GB::presentation::make_output_message(
      "Starting segment movement...\nSegment completed.\n"));
  return true;
}

const CommandDescriptor segment_cmd{
    .name = "@@segment",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .syntax = "@@segment [seg_num]",
    .description = "Trigger segment movement (deity only)",
    .handler = &segment,
    .transactional = false,
};

}  // namespace GB::commands
