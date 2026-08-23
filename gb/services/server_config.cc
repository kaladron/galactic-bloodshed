// SPDX-License-Identifier: Apache-2.0

/// \file server_config.cc
/// \brief Implementation of server configuration, argument parsing, and
/// initialization.

module;

import dallib;
import gblib;
import std;

module server_config;

ServerConfig parse_server_args(int argc, const char* const* argv) {
  ServerConfig config{};
  switch (argc) {
    case 2:
      config.port = std::stoi(argv[1]);
      config.update_time = std::chrono::minutes(DEFAULT_UPDATE_TIME);
      config.segments = MOVES_PER_UPDATE;
      break;
    case 3:
      config.port = std::stoi(argv[1]);
      config.update_time = std::chrono::minutes(std::stoi(argv[2]));
      config.segments = MOVES_PER_UPDATE;
      break;
    case 4:
      config.port = std::stoi(argv[1]);
      config.update_time = std::chrono::minutes(std::stoi(argv[2]));
      config.segments = std::stoi(argv[3]);
      break;
    default:
      config.port = GB_PORT;
      config.update_time = DEFAULT_UPDATE_TIME;
      config.segments = MOVES_PER_UPDATE;
      break;
  }
  return config;
}

void initialize_schedule_state(ServerState& state, const ServerConfig& config,
                               std::time_t current_time) {
  state.update_time_minutes = config.update_time.count();
  state.segments = config.segments;

  if (state.next_update_time == 0) {
    state.next_update_time = current_time + (state.update_time_minutes * 60);
  }
  if (state.segments <= 1) {
    state.next_segment_time = current_time + (144 * 3600);
  } else {
    if (state.next_segment_time == 0) {
      state.next_segment_time =
          current_time + (state.update_time_minutes * 60 / state.segments);
    }
    if (state.next_segment_time < current_time) {
      state.next_segment_time = state.next_update_time;
      state.nsegments_done = state.segments;
    }
  }
}

void initialize_block_data(EntityManager& entity_manager) {
  for (auto race_handle : RaceList(entity_manager)) {
    const auto& race = race_handle.read();
    const player_t i = race.Playernum;
    auto block_handle = entity_manager.get_block(i.value);
    setbit(block_handle->invite, i);
    setbit(block_handle->pledge, i);
  }
}
