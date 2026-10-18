// SPDX-License-Identifier: Apache-2.0

/// \file time.cc
/// \brief Time and schedule command implementations.

module commands;

import gb.entities;
import gb.presentation;
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
  GB::presentation::TimeViewModel vm{
      .start_line = state->start_line(),
      .update_line = state->update_line(),
      .segment_line = state->segment_line(),
      .current_time = format_timestamp(clk),
  };
  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

bool schedule(const command_t&, GameObj& g) {
  std::time_t clk = std::time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return false;
  }
  GB::presentation::ScheduleViewModel vm{
      .update_time_minutes = state->update_time_minutes,
      .segments = state->segments,
      .current_time = format_timestamp(clk),
      .next_segment_number = state->nsegments_done == state->segments
                                 ? 1
                                 : state->nsegments_done + 1,
      .next_segment_time = format_timestamp(state->next_segment_time),
      .next_update_number = state->nupdates_done + 1,
      .next_update_time = format_timestamp(state->next_update_time),
  };
  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
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
