// SPDX-License-Identifier: Apache-2.0

/// \file server_config.cc
/// \brief Implementation of server configuration, argument parsing, and
/// initialization.

module;

import dallib;
import gb.entities;
import gb.services;
import std;

module server_config;

ServerConfig parse_server_args(int argc, const char* const* argv) {
  ServerConfig config{};
  int positional_index = 0;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      config.show_help = true;
      return config;
    }
    if (arg == "-p" || arg == "--port") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Error: Option \"{}\" requires an argument.",
                     arg);
        config.has_error = true;
        return config;
      }
      try {
        config.port = std::stoi(argv[++i]);
      } catch (const std::exception&) {
        std::println(std::cerr, "Error: Invalid port number \"{}\".", argv[i]);
        config.has_error = true;
        return config;
      }
    } else if (arg.starts_with("--port=")) {
      auto val = arg.substr(std::string_view("--port=").size());
      try {
        config.port = std::stoi(std::string(val));
      } catch (const std::exception&) {
        std::println(std::cerr, "Error: Invalid port number \"{}\".", val);
        config.has_error = true;
        return config;
      }
    } else if (arg == "-d" || arg == "--database" || arg == "--db") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Error: Option \"{}\" requires an argument.",
                     arg);
        config.has_error = true;
        return config;
      }
      config.db_path = argv[++i];
    } else if (arg.starts_with("--database=")) {
      config.db_path = arg.substr(std::string_view("--database=").size());
    } else if (arg.starts_with("--db=")) {
      config.db_path = arg.substr(std::string_view("--db=").size());
    } else if (arg.starts_with("-")) {
      std::println(std::cerr, "Error: Unknown option \"{}\".", arg);
      config.has_error = true;
      return config;
    } else {
      // Positional argument
      try {
        switch (positional_index) {
          case 0:
            config.port = std::stoi(std::string(arg));
            break;
          case 1:
            config.update_time =
                std::chrono::minutes(std::stoi(std::string(arg)));
            break;
          case 2:
            config.segments = std::stoi(std::string(arg));
            break;
          default:
            std::println(std::cerr,
                         "Error: Unexpected positional argument \"{}\".", arg);
            config.has_error = true;
            return config;
        }
        positional_index++;
      } catch (const std::exception&) {
        std::println(std::cerr, "Error: Invalid numerical argument \"{}\".",
                     arg);
        config.has_error = true;
        return config;
      }
    }
  }
  return config;
}

void print_server_usage(const char* prog_name) {
  const char* name =
      (prog_name != nullptr && prog_name[0] != '\0') ? prog_name : "GB";
  std::println(std::cout, "Usage: {} [options] [port] [update_time] [segments]",
               name);
  std::println(std::cout, "");
  std::println(std::cout, "Options:");
  std::println(std::cout,
               "  -p, --port <port>           Port to listen on (default: {})",
               GB_PORT);
  std::println(std::cout,
               "  -d, --database, --db <path> Path to SQLite database "
               "(default: {}gb.db)",
               PKGSTATEDIR);
  std::println(std::cout,
               "  -h, --help                  Display this help message and "
               "exit");
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
  for (const Race& race : RaceList::readonly(entity_manager)) {
    const player_t i = race.Playernum;
    try {
      entity_manager.mutate_block(i.value, [&](struct block& b) {
        b.invite_player(i);
        b.pledge_player(i);
      });
    } catch (const EntityNotFoundError&) {
    }
  }
}
