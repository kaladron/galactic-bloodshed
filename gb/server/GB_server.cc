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
import gb.server;

int main(int argc, char** argv) {
  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};

  std::println(std::cout, "      ***   Galactic Bloodshed ver {0} ***",
               GB_VERSION);
  std::println(std::cout, "");
  std::time_t clk = std::time(nullptr);
  std::print("      {0}", std::ctime(&clk));
  if (EXTERNAL_TRIGGER) {
    std::println(std::cout, "      The update  password is '%s'.",
                 UPDATE_PASSWORD);
    std::println(std::cout, "      The segment password is '%s'.",
                 SEGMENT_PASSWORD);
  }
  ServerConfig config = parse_server_args(argc, argv);
  entity_manager.mutate_server_state([&](ServerState& state) {
    initialize_schedule_state(state, config, clk);
  });

  entity_manager.with_server_state([&](const ServerState& state) {
    std::cerr << "      Port " << config.port << '\n';
    std::cerr << "      " << config.update_time << " minutes between updates"
              << '\n';
    std::cerr << "      " << state.segments << " segments/update" << '\n';
    set_server_start_time(clk);

    // Print initial schedule status
    std::print(stderr, "Last Update {:3d} : {}", 0, std::ctime(&clk));
    std::print(stderr, "Last Segment {0:2d} : {1}", state.nsegments_done,
               std::ctime(&clk));
    srandom(getpid());
    std::print(stderr, "      Next Update {0}  : {1}", 1,
               std::ctime(&state.next_update_time));
    std::print(stderr, "      Next Segment   : {0}",
               std::ctime(&state.next_segment_time));
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
