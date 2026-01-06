// SPDX-License-Identifier: Apache-2.0

export module gblib:doturncmd;

import :gameobj;
import :services;
import :types;
import std;

class SessionRegistry;

export void do_turn(EntityManager&, SessionRegistry&, int);
export void do_next_thing(EntityManager&, SessionRegistry&);
export void do_update(EntityManager&, SessionRegistry&, bool = false);
export void do_segment(EntityManager&, SessionRegistry&, int, int);
export void handle_victory(EntityManager&);
export void compute_power_blocks(EntityManager&);

/// Schedule status info for display commands
export struct ScheduleInfo {
  std::string start_buf;    // "Server started  : <time>"
  std::string update_buf;   // "Last Update N : <time>"
  std::string segment_buf;  // "Last Segment N : <time>"
  unsigned int nupdates_done;
  std::time_t last_update_time;
  std::time_t last_segment_time;
};

/// Get current schedule status for display
export const ScheduleInfo& get_schedule_info();

/// Set server start time (called once at startup)
export void set_server_start_time(std::time_t start_time);
