// SPDX-License-Identifier: Apache-2.0

/// \file segment.cc
/// \brief Trigger segment movement (deity only).

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool segment(const command_t& argv, GameObj& g) {
  int seg_num = 0;
  if (argv.size() > 1) {
    auto [ptr, ec] = std::from_chars(argv[1].data(),
                                     argv[1].data() + argv[1].size(), seg_num);
    if (ec != std::errc{}) {
      g.out << "Invalid segment number.\n";
      return false;
    }
  }

  g.out << "Starting segment movement...\n";
  g.session_registry.flush_all();
  do_segment(g.entity_manager, g.session_registry, 1, seg_num);
  g.out << "Segment completed.\n";
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
};

}  // namespace GB::commands
