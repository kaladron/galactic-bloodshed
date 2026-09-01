// SPDX-License-Identifier: Apache-2.0

/// \file server_config.cppm
/// \brief Server configuration, CLI argument parsing, and startup schedule
/// helpers.

module;

import std;

export module server_config;

import dallib;
import gb.entities;
import gb.services;

export struct ServerConfig {
  std::string db_path{PKGSTATEDIR "gb.db"};
  int port{GB_PORT};
  std::chrono::minutes update_time{DEFAULT_UPDATE_TIME};
  int segments{MOVES_PER_UPDATE};
  bool show_help{false};
  bool has_error{false};
};

export ServerConfig parse_server_args(int argc, const char* const* argv);
export void print_server_usage(const char* prog_name);
export void initialize_schedule_state(ServerState& state,
                                      const ServerConfig& config,
                                      std::time_t current_time);
export void initialize_block_data(EntityManager& entity_manager);
