// SPDX-License-Identifier: Apache-2.0

/// \file GB_server.cc
/// \brief Main game server executable.

import std;
import asio;
import dallib;
import gb.entities;
import gb.log;
import gb.services;
import gb.mechanics;
import gb.turn;
import gb.server;

int main(int argc, char** argv) {
  ServerConfig config = parse_server_args(argc, argv);
  if (config.show_version) {
    std::println(std::cout, "GB {}", GB_VERSION);
    return 0;
  }
  if (config.show_help) {
    print_server_usage(argv[0]);
    return 0;
  }
  if (config.has_error) {
    print_server_usage(argv[0]);
    return 1;
  }

  bool server_logged_fatal = false;
  try {
    // Create Database and EntityManager for dependency injection
    Database database{config.db_path};
    EntityManager entity_manager{database};

    const std::time_t clk = std::time(nullptr);
    const std::string banner = format_startup_banner(clk);
    for (auto line_range : banner | std::views::split('\n')) {
      std::string_view line{line_range};
      if (const auto first = line.find_first_not_of(' ');
          first != std::string_view::npos) {
        GB::log::info("{}", line.substr(first));
      }
    }

    entity_manager.mutate_server_state([&](ServerState& state) {
      initialize_schedule_state(state, config, clk);
    });

    entity_manager.with_server_state([&](const ServerState& state) {
      GB::log::info("Port {}", config.port);
      GB::log::info("Database {}", config.db_path);
      GB::log::info("{} minutes between updates", config.update_time.count());
      GB::log::info("{} segments/update", state.segments);

      // Log initial schedule status
      GB::log::info("Last Update {:3d} : {}", 0, format_timestamp(clk));
      GB::log::info("Last Segment {:2d} : {}", state.nsegments_done,
                    format_timestamp(clk));
      GB::log::info("Next Update {}  : {}", 1,
                    format_timestamp(state.next_update_time));
      GB::log::info("Next Segment   : {}",
                    format_timestamp(state.next_segment_time));
    });

    // Verify universe is initialized (created by makeuniv)
    try {
      entity_manager.with_universe([](const Universe&) {});
    } catch (const EntityNotFoundError&) {
      GB::log::fatal(
          "Universe not initialized in '{}'; please run 'makeuniv' to create "
          "the game universe.",
          config.db_path);
      return 1;
    }

    // Initialize game data structures
    initialize_block_data(entity_manager);  // Ensure self-invite/self-pledge
    compute_power_blocks(entity_manager);   // Calculate alliance power stats

    // Start server using Asio-based Server class
    asio::io_context io;
    Server server(io, config.port, entity_manager);
    post(entity_manager, "Server started\n", NewsType::ANNOUNCE);
    try {
      server.run();
    } catch (...) {
      server_logged_fatal = server.has_fatal_error();
      throw;
    }

    GB::log::info("Going down.");
    return 0;
  } catch (const std::exception& e) {
    if (!server_logged_fatal) {
      GB::log::fatal("Unhandled fatal exception: {}", e.what());
    }
    return 1;
  } catch (...) {
    if (!server_logged_fatal) {
      GB::log::fatal("Unhandled unknown fatal exception");
    }
    return 1;
  }
}
