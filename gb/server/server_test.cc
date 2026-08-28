// SPDX-License-Identifier: Apache-2.0

/// \file server_test.cc
/// \brief Comprehensive unit tests for Server class implementing
/// SessionRegistry and async network handling.

import asio;
import auth;
import commands;
import dallib;
import gblib;
import notification;
import server;
import session;
import test;
import std;

namespace {

void test_server_initialization_and_registry_primitives() {
  TestContext ctx;
  asio::io_context io;
  Server server(io, 0, ctx.em);

  test::expect_eq(server.session_count(), 0);
  test::expect_false(server.is_connected(1, 0));
  test::expect_true(server.get_connected_sessions().empty());
  test::expect_false(server.update_in_progress());

  server.set_update_in_progress(true);
  test::expect_true(server.update_in_progress());
  server.set_update_in_progress(false);
  test::expect_false(server.update_in_progress());

  // Notification methods should safely handle empty session list
  server.notify_race(1, "Broadcast message\n");
  test::expect_false(server.notify_player(1, 0, "Personal message\n"));
  server.flush_all();

  server.shutdown();
}

void test_server_network_lifecycle_and_session_handling() {
  TestContext ctx;

  Race race{};
  race.Playernum = 1;
  race.name = "ServerTestRace";
  race.password = "raceword";
  race.governor[0].name = "Gov0";
  race.governor[0].password = "govword";
  race.governor[0].deflevel = ScopeLevel::LEVEL_UNIV;
  race.governor[0].defsystem = 0;
  race.governor[0].defplanetnum = 0;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);

    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 0;
    sdata.name = "Sol";
    Star star{sdata};
    star_repo.save(star);

    UniverseRepository univ_repo(store);
    universe_struct u{};
    u.id = 1;
    u.numstars = 1;
    univ_repo.save(u);
  }

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();

  // Connect client socket to server's dynamically assigned port
  asio::ip::tcp::socket client_socket(io);
  client_socket.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));

  // Run io to accept connection and dispatch welcome message
  io.poll();

  test::expect_eq(server.session_count(), 1);

  // Read welcome message
  std::array<char, 512> read_buf{};
  std::size_t n = client_socket.read_some(asio::buffer(read_buf));
  std::string welcome_msg(read_buf.data(), n);
  test::expect_contains(welcome_msg, "Welcome to Galactic Bloodshed");

  // Send credentials
  std::string creds = "raceword govword\n";
  client_socket.write_some(asio::buffer(creds));

  // Run io to read input from socket
  io.poll();

  // Test registry methods with connected session
  server.notify_race(1, "Race announcement\n");
  server.flush_all();
  io.poll();

  server.shutdown();
  test::expect_eq(server.session_count(), 0);
}

}  // namespace

int main() {
  test_server_initialization_and_registry_primitives();
  test_server_network_lifecycle_and_session_handling();

  std::println(std::cout, "✓ server_test passed!");
  return 0;
}
