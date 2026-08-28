// SPDX-License-Identifier: Apache-2.0

/// \file time.cc
/// \brief Time and schedule command implementations.

module commands;

import gb.entities;
import gb.services;
import std;

namespace GB::commands {
bool time(const command_t&, GameObj& g) {
  std::time_t clk = std::time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return false;
  }
  const auto& sched = get_schedule_info();
  g.out << sched.start_buf;
  g.out << sched.update_buf;
  g.out << sched.segment_buf;
  g.out << std::format("Current time    : {0}", std::ctime(&clk));
  return true;
}

bool schedule(const command_t&, GameObj& g) {
  std::time_t clk = std::time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return false;
  }
  const auto& sched = get_schedule_info();
  g.out << std::format("{0} minute update intervals\n",
                       state->update_time_minutes);
  g.out << std::format("{0} movement segments per update\n", state->segments);
  g.out << std::format("Current time    : {0}", std::ctime(&clk));
  g.out << std::format(
      "Next Segment {0:2d} : {1}",
      state->nsegments_done == state->segments ? 1 : state->nsegments_done + 1,
      std::ctime(&state->next_segment_time));
  g.out << std::format("Next Update {0:3d} : {1}", sched.nupdates_done + 1,
                       std::ctime(&state->next_update_time));
  return true;
}

const CommandDescriptor time_cmd{
    .name = "time",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "time",
    .description = "Display server update status and current time",
    .handler = &time,
};

const CommandDescriptor schedule_cmd{
    .name = "schedule",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "schedule",
    .description = "Display turn update and movement segment schedule",
    .handler = &schedule,
};

}  // namespace GB::commands
