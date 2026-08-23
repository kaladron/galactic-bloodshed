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
import gblib;
import server;
import server_config;

int main(int argc, char** argv) {
  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};

  // Get server state handle (will auto-save on scope exit)
  auto server_state_handle = entity_manager.get_server_state();
  auto& state = *server_state_handle;

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
  initialize_schedule_state(state, config, clk);

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

  // Verify universe is initialized (created by makeuniv)
  const auto* universe = entity_manager.peek_universe();
  if (!universe) {
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

  // Save final state before shutdown
  server_state_handle.save();

  std::println(std::cout, "Going down.");
  return 0;
}
