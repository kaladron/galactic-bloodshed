// SPDX-License-Identifier: Apache-2.0

/// \file server_test.cc
/// \brief Comprehensive unit tests for Server class implementing
/// SessionRegistry and async network handling.

import asio;
import auth;
import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.server;
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

  test::expect_eq(server.session_count(), 0u);
  test::expect_false(server.is_connected(1, 1));
  test::expect_true(server.get_connected_sessions().empty());
  test::expect_false(server.update_in_progress());
  test::expect_true(&server.entity_manager() == &ctx.em);

  server.set_update_in_progress(true);
  test::expect_true(server.update_in_progress());
  server.set_update_in_progress(false);
  test::expect_false(server.update_in_progress());

  test::expect_false(server.has_pending_turn());
  server.request_next_thing();
  test::expect_true(server.has_pending_turn());
  server.clear_pending_turn();
  test::expect_false(server.has_pending_turn());

  // Notification methods should safely handle empty session list
  server.notify_race(1, "Broadcast message\n");
  test::expect_false(server.notify_player(1, 1, "Personal message\n"));
  server.flush_all();

  server.shutdown();
}

void setup_test_universe(TestContext& ctx) {
  Race race{};
  race.Playernum = 1;
  race.name = "ServerTestRace";
  race.password = "raceword";
  race.God = true;
  race.leader().name = "Gov1";
  race.leader().password = "govword";
  race.leader().deflevel = ScopeLevel::LEVEL_UNIV;
  race.leader().defsystem = 1;
  race.leader().defplanetnum = 1;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  StarRepository star_repo(store);
  star_struct sdata{};
  sdata.star_id = 1;
  sdata.name = "Sol";
  Star star{sdata};
  star_repo.save(star);

  UniverseRepository univ_repo(store);
  universe_struct u{};
  univ_repo.save(u);

  ServerStateRepository state_repo(store);
  ServerState state{};
  state.next_update_time = std::time(nullptr) + 3600;
  state.next_segment_time = std::time(nullptr) + 1800;
  state.segments = 3;
  state.nsegments_done = 0;
  state_repo.save(state);
}

std::string drain_socket(asio::ip::tcp::socket& socket) {
  std::string result;
  std::array<char, 1024> buf{};
  asio::error_code ec;
  std::size_t first = socket.read_some(asio::buffer(buf), ec);
  if (!ec && first > 0) {
    result.append(buf.data(), first);
  }
  while (socket.available(ec) > 0 && !ec) {
    std::size_t n = socket.read_some(asio::buffer(buf), ec);
    if (ec || n == 0) break;
    result.append(buf.data(), n);
  }
  return result;
}

void test_server_network_lifecycle_and_session_handling() {
  TestContext ctx;
  setup_test_universe(ctx);

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();
  // Calling start() a second time is a safe no-op
  server.start();

  // 1. Failed authentication attempt disconnects with "Goodbye!\n"
  {
    asio::ip::tcp::socket bad_client(io);
    bad_client.connect(asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(),
                                               server.port()));
    io.poll();
    test::expect_eq(server.session_count(), 1u);

    std::string welcome = drain_socket(bad_client);
    test::expect_contains(welcome, "Welcome to Galactic Bloodshed");

    std::string bad_creds = "wrongpass badgov\n";
    bad_client.write_some(asio::buffer(bad_creds));
    io.poll();
    server.process_commands();
    io.poll();

    std::string bad_reply = drain_socket(bad_client);
    test::expect_contains(bad_reply, "Connection refused.");
    test::expect_contains(bad_reply, "Goodbye!");
    test::expect_eq(server.session_count(), 0u);
  }

  // 2. Valid login, SessionRegistry queries, notifications, commands, and quit
  asio::ip::tcp::socket client_socket(io);
  client_socket.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  test::expect_eq(server.session_count(), 1u);

  std::string welcome_msg = drain_socket(client_socket);
  test::expect_contains(welcome_msg, "Welcome to Galactic Bloodshed");

  // Send valid credentials and process login
  std::string creds = "raceword govword\n";
  client_socket.write_some(asio::buffer(creds));
  io.poll();
  server.on_timer();
  io.poll();

  test::expect_true(server.is_connected(1, 1));
  test::expect_false(server.is_connected(1, 2));
  auto connected = server.get_connected_sessions();
  test::expect_eq(connected.size(), 1u);
  test::expect_eq(connected[0].player.value, 1);
  test::expect_eq(connected[0].governor.value, 1);
  test::expect_true(connected[0].god);

  std::string login_reply = drain_socket(client_socket);
  test::expect_contains(login_reply, "ServerTestRace");

  // Test notify_player and notify_race with connected session
  test::expect_true(server.notify_player(1, 1, "Direct alert\n"));
  test::expect_false(server.notify_player(2, 1, "Other race alert\n"));
  server.notify_race(1, "Race announcement\n");

  // Suppressed while update_in_progress is true
  server.set_update_in_progress(true);
  test::expect_false(server.notify_player(1, 1, "Suppressed direct\n"));
  server.notify_race(1, "Suppressed race\n");
  server.set_update_in_progress(false);

  server.flush_all();
  io.poll();

  std::string notify_reply = drain_socket(client_socket);
  test::expect_contains(notify_reply, "Direct alert");
  test::expect_contains(notify_reply, "Race announcement");

  // Execute a valid command (`cs /Sol`) and an unknown command (`unknown_xyz`)
  std::string cmds = "cs /Sol\nunknown_xyz\n";
  client_socket.write_some(asio::buffer(cmds));
  io.poll();
  server.process_commands();
  io.poll();

  std::string cmd_reply = drain_socket(client_socket);
  test::expect_contains(cmd_reply, "'unknown_xyz':illegal command error.");
  test::expect_contains(cmd_reply, "/Sol");

  // Disconnect via `quit` command
  std::string quit_cmd = "quit\n";
  client_socket.write_some(asio::buffer(quit_cmd));
  io.poll();
  server.process_commands();
  io.poll();

  std::string quit_reply = drain_socket(client_socket);
  test::expect_eq(quit_reply, "Goodbye!\n");
  test::expect_eq(server.session_count(), 0u);
  test::expect_false(server.is_connected(1, 1));

  server.shutdown();
}

void test_server_quotas_idle_timeout_and_turn_events() {
  TestContext ctx;
  setup_test_universe(ctx);

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();

  // Connect and log in a session
  asio::ip::tcp::socket client_socket(io);
  client_socket.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(client_socket);

  std::string creds = "raceword govword\n";
  client_socket.write_some(asio::buffer(creds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(client_socket);

  test::expect_true(server.is_connected(1, 1));

  // Replenish command quotas across slices
  server.update_quotas(std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(COMMAND_TIME_MSEC * 2 + 50));

  // Non-idle check does not disconnect active session
  server.check_idle_sessions(std::time(nullptr) + 10);
  test::expect_eq(server.session_count(), 1u);

  // Idle timeout check disconnects session exceeding IDLE_TIMEOUT_SECONDS
  server.check_idle_sessions(std::time(nullptr) + IDLE_TIMEOUT_SECONDS + 10);
  std::string timeout_reply = drain_socket(client_socket);
  test::expect_contains(timeout_reply,
                        "Connection timed out due to inactivity.");
  test::expect_eq(server.session_count(), 0u);

  // Turn events: segment trigger and update trigger
  const auto* state = ctx.em.peek_server_state();
  test::expect_true(state != nullptr);
  std::time_t seg_time = state->next_segment_time;
  server.check_turn_events(seg_time + 1);

  state = ctx.em.peek_server_state();
  std::time_t upd_time = state->next_update_time;
  server.check_turn_events(upd_time + 1);

  // Explicit request_next_thing() pending turn trigger
  server.request_next_thing();
  test::expect_true(server.has_pending_turn());
  server.check_turn_events(std::time(nullptr));
  test::expect_false(server.has_pending_turn());

  server.shutdown();
}

}  // namespace

int main() {
  test_server_initialization_and_registry_primitives();
  test_server_network_lifecycle_and_session_handling();
  test_server_quotas_idle_timeout_and_turn_events();

  std::println(std::cout, "✓ server_test passed!");
  return 0;
}
