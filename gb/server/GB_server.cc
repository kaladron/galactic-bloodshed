// SPDX-License-Identifier: Apache-2.0

/// \file GB_server.cc
/// \brief Main game server executable.

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

import std;
import asio;
import dallib;
import gb.entities;
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

  // Create Database and EntityManager for dependency injection
  Database database{config.db_path};
  EntityManager entity_manager{database};

  std::println(std::cout, "      ***   Galactic Bloodshed v{0} ***",
               GB_VERSION);
  std::println(std::cout, "");
  std::time_t clk = std::time(nullptr);
  std::println("      {}", format_timestamp(clk));
  if (EXTERNAL_TRIGGER) {
    std::println(std::cout, "      The update  password is '%s'.",
                 UPDATE_PASSWORD);
    std::println(std::cout, "      The segment password is '%s'.",
                 SEGMENT_PASSWORD);
  }
  entity_manager.mutate_server_state([&](ServerState& state) {
    initialize_schedule_state(state, config, clk);
  });

  entity_manager.with_server_state([&](const ServerState& state) {
    std::cerr << "      Port " << config.port << '\n';
    std::cerr << "      Database " << config.db_path << '\n';
    std::cerr << "      " << config.update_time << " minutes between updates"
              << '\n';
    std::cerr << "      " << state.segments << " segments/update" << '\n';

    // Print initial schedule status
    std::println(stderr, "Last Update {:3d} : {}", 0, format_timestamp(clk));
    std::println(stderr, "Last Segment {:2d} : {}", state.nsegments_done,
                 format_timestamp(clk));
    srandom(getpid());
    std::println(stderr, "      Next Update {}  : {}", 1,
                 format_timestamp(state.next_update_time));
    std::println(stderr, "      Next Segment   : {}",
                 format_timestamp(state.next_segment_time));
  });

  // Verify universe is initialized (created by makeuniv)
  try {
    entity_manager.with_universe([](const universe_struct&) {});
  } catch (const EntityNotFoundError&) {
    std::println(stderr, "\nERROR: Universe not initialized!");
    std::println(stderr, "Please run 'makeuniv' to create the game universe.");
    return 1;
  }

  // Initialize game data structures
  initialize_block_data(entity_manager);  // Ensure self-invite/self-pledge
  compute_power_blocks(entity_manager);   // Calculate alliance power stats

  // Start server using new Asio-based Server class
  asio::io_context io;
  Server server(io, config.port, entity_manager);
  post(entity_manager, "Server started\n", NewsType::ANNOUNCE);
  server.run();

  std::println(std::cout, "Going down.");
  return 0;
}
