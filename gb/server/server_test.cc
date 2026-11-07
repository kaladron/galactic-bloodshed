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
  race.appoint_governor({.name = "Gov2", .password = "govword2"});

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  Race mortal{};
  mortal.Playernum = 2;
  mortal.name = "MortalRace";
  mortal.password = "mortalpass";
  mortal.God = false;
  mortal.leader().name = "MortalGov";
  mortal.leader().password = "mortalword";
  mortal.leader().deflevel = ScopeLevel::LEVEL_STAR;
  mortal.leader().defsystem = 1;
  mortal.leader().defplanetnum = 1;
  races.save(mortal);

  PowerRepository powers(store);
  powers.save(Power{{.id = 1}});
  powers.save(Power{{.id = 2}});

  UniverseRepository univ_repo(store);
  Universe u{};
  univ_repo.save(u);

  ctx.create_star("Sol", 1).with_position({300.0, 400.0}).build();

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

std::string drain_socket_available(asio::ip::tcp::socket& socket) {
  std::string result;
  std::array<char, 1024> buf{};
  asio::error_code ec;
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

  // Verify GameObj viewport state (center and zoom) persists across commands
  std::string viewport_set_cmds = "cs /\ncenter /Sol\nzoom 2.5\n";
  client_socket.write_some(asio::buffer(viewport_set_cmds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(client_socket);

  std::string viewport_query_cmd = "zoom\n";
  client_socket.write_some(asio::buffer(viewport_query_cmd));
  io.poll();
  server.process_commands();
  io.poll();

  std::string viewport_reply = drain_socket(client_socket);
  test::expect_contains(viewport_reply,
                        "Zoom value 2.5, lastx = 300, lasty = 400.");

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

void test_server_slow_client_flush_reaping() {
  TestContext ctx;
  setup_test_universe(ctx);

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();

  // Connect two active sessions (Gov 1 and Gov 2 of Race 1)
  asio::ip::tcp::socket client1(io);
  client1.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  asio::ip::tcp::socket client2(io);
  client2.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(client1);
  (void)drain_socket(client2);

  std::string creds1 = "raceword govword\n";
  client1.write_some(asio::buffer(creds1));
  std::string creds2 = "raceword govword2\n";
  client2.write_some(asio::buffer(creds2));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(client1);
  (void)drain_socket(client2);

  test::expect_eq(server.session_count(), 2u);
  test::expect_true(server.is_connected(1, 1));
  test::expect_true(server.is_connected(1, 2));

  // Queue a large payload on client1 exceeding MAX_WRITE_QUEUE_SIZE without
  // draining io
  server.notify_player(1, 1, std::string(MAX_WRITE_QUEUE_SIZE + 1, 'Z'));
  server.flush_all();
  test::expect_eq(server.session_count(), 2u);

  // Broadcast to both governors and flush_all(); client1 overflows and
  // disconnects mid-iteration while client2 receives the broadcast cleanly
  server.notify_race(1, "Broadcast after slow client overflow\n");
  server.flush_all();

  test::expect_eq(server.session_count(), 1u);
  test::expect_false(server.is_connected(1, 1));
  test::expect_true(server.is_connected(1, 2));

  io.poll();
  std::string reply2 = drain_socket(client2);
  test::expect_contains(reply2, "Broadcast after slow client overflow\n");

  server.shutdown();
}

void test_server_deity_departure_broadcast() {
  TestContext ctx;
  setup_test_universe(ctx);

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();

  // 1. Connect a mortal client (Race 2, MortalRace)
  asio::ip::tcp::socket mortal_client(io);
  mortal_client.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(mortal_client);

  std::string mortal_creds = "mortalpass mortalword\n";
  mortal_client.write_some(asio::buffer(mortal_creds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(mortal_client);
  test::expect_true(server.is_connected(2, 1));

  // 2. Connect Deity client (Race 1, ServerTestRace)
  asio::ip::tcp::socket deity_client(io);
  deity_client.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(deity_client);

  std::string deity_creds = "raceword govword\n";
  deity_client.write_some(asio::buffer(deity_creds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(deity_client);
  test::expect_true(server.is_connected(1, 1));

  // Mortal receives arrival broadcast
  std::string arrival_broadcast = drain_socket(mortal_client);
  test::expect_contains(
      arrival_broadcast,
      "*** Deity ServerTestRace \"Gov1\" [1,1] has entered the universe. ***");

  // 3. Deity disconnects via `quit`
  std::string deity_quit = "quit\n";
  deity_client.write_some(asio::buffer(deity_quit));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(deity_client);
  test::expect_false(server.is_connected(1, 1));

  // Mortal receives departure broadcast
  std::string departure_broadcast = drain_socket(mortal_client);
  test::expect_contains(
      departure_broadcast,
      "*** Deity ServerTestRace \"Gov1\" [1,1] has departed the universe. ***");

  // 4. Another mortal client connects, logs in, and disconnects: NO departure
  // broadcast sent to first mortal
  asio::ip::tcp::socket mortal2_client(io);
  mortal2_client.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(mortal2_client);

  // Appoint second governor for mortal race
  ctx.em.mutate_race(2, [](Race& r) {
    r.appoint_governor({.name = "MortalGov2", .password = "mortalword2"});
  });
  std::string mortal2_creds = "mortalpass mortalword2\n";
  mortal2_client.write_some(asio::buffer(mortal2_creds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(mortal2_client);
  test::expect_true(server.is_connected(2, 2));

  // Clear any pending messages on first mortal
  (void)drain_socket_available(mortal_client);

  // Mortal 2 quits
  std::string mortal2_quit = "quit\n";
  mortal2_client.write_some(asio::buffer(mortal2_quit));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(mortal2_client);
  test::expect_false(server.is_connected(2, 2));

  // Mortal 1 receives NO departure broadcast from Mortal 2
  std::string mortal1_after_mortal_quit = drain_socket_available(mortal_client);
  test::expect_false(
      mortal1_after_mortal_quit.contains("has departed the universe"));

  // 5. Connect Deity again, then trigger server.shutdown(): NO departure
  // broadcast sent
  asio::ip::tcp::socket deity2_client(io);
  deity2_client.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  (void)drain_socket(deity2_client);

  deity2_client.write_some(asio::buffer(deity_creds));
  io.poll();
  server.process_commands();
  io.poll();
  (void)drain_socket(deity2_client);
  test::expect_true(server.is_connected(1, 1));
  (void)drain_socket(mortal_client);  // Drain arrival broadcast

  // Server shutdown with connected deity
  server.shutdown();
  io.poll();

  std::string mortal_after_shutdown = drain_socket_available(mortal_client);
  test::expect_false(
      mortal_after_shutdown.contains("has departed the universe"));
}

void test_server_json_mode_single_session_stanza() {
  TestContext ctx;
  setup_test_universe(ctx);
  ctx.em.mutate_server_state([](ServerState& state) {
    state.update_time_minutes = 60;
    state.record_server_start(1700000000);
    state.record_update_completed(1700003600, false);
    state.record_segment_completed(1700005400);
  });

  asio::io_context io;
  Server server(io, 0, ctx.em);
  server.start();

  asio::ip::tcp::socket client(io);
  client.connect(
      asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), server.port()));
  io.poll();
  test::expect_eq(server.session_count(), 1u);
  (void)drain_socket(client);  // Drain ASCII welcome message

  auto send_cmd = [&](std::string_view line) {
    std::string payload = std::format("{}\n", line);
    client.write_some(asio::buffer(payload));
    io.poll();
    server.process_commands();
    io.poll();
    return drain_socket(client);
  };

  // 1. Connect in JSON mode: exactly 1 "session" and 1 "race", 0 "message"
  const std::string login_reply = send_cmd("raceword govword json");
  test::expect_count(login_reply, R"("type":"session")", 1);
  test::expect_count(login_reply, R"("type":"race")", 1);
  test::expect_count(login_reply, R"("type":"message")", 0);
  test::expect_contains(login_reply, R"("race_name":"ServerTestRace")");

  // 2. Command 'cs /Sol': scope change reflected in single canonical session
  const std::string cs_reply = send_cmd("cs /Sol");
  test::expect_count(cs_reply, R"("type":"session")", 1);
  test::expect_count(cs_reply, R"("type":"message")", 0);
  test::expect_contains(cs_reply, R"("star":"Sol")");
  test::expect_contains(cs_reply, R"("path":"/Sol")");

  // 3. Command 'center /Sol': viewport center reflected in single session
  const std::string center_reply = send_cmd("center /Sol");
  test::expect_count(center_reply, R"("type":"session")", 1);
  test::expect_count(center_reply, R"("type":"message")", 0);
  test::expect_contains(center_reply, R"("universe_center":{"x":300,"y":400})");

  // 4. Command 'time': ServerState timestamps reflected in single session clock
  const std::string time_reply = send_cmd("time");
  test::expect_count(time_reply, R"("type":"session")", 1);
  test::expect_count(time_reply, R"("type":"message")", 0);
  test::expect_contains(time_reply, R"("server_start_time":1700000000)");
  test::expect_contains(time_reply, R"("last_update_time":1700003600)");
  test::expect_contains(time_reply, R"("last_segment_time":1700005400)");

  // 5. Command 'schedule': schedule fields reflected in single session clock
  const std::string sched_reply = send_cmd("schedule");
  test::expect_count(sched_reply, R"("type":"session")", 1);
  test::expect_count(sched_reply, R"("type":"message")", 0);
  test::expect_contains(sched_reply, R"("segments_per_update":3)");
  test::expect_contains(sched_reply, R"("next_segment_time":)");
  test::expect_contains(sched_reply, R"("next_update_time":)");

  // 6. Command 'toggle': governor toggles reflected in single session
  const std::string toggle_reply = send_cmd("toggle");
  test::expect_count(toggle_reply, R"("type":"session")", 1);
  test::expect_count(toggle_reply, R"("type":"message")", 0);
  test::expect_contains(toggle_reply, R"("toggles":)");

  // 7. Command 'highlight 2': 1 dirty race stanza + 1 session stanza (0
  // messages)
  const std::string highlight_reply = send_cmd("highlight 2");
  test::expect_count(highlight_reply, R"("type":"session")", 1);
  test::expect_count(highlight_reply, R"("type":"race")", 1);
  test::expect_count(highlight_reply, R"("type":"message")", 0);
  test::expect_contains(highlight_reply, R"("highlight":2)");

  // 8. Command 'emulate 2 1': 1 confirmation message + 1 session stanza with
  // emulated identity
  const std::string emulate_reply = send_cmd("emulate 2 1");
  test::expect_count(emulate_reply, R"("type":"session")", 1);
  test::expect_count(emulate_reply, R"("type":"message")", 1);
  test::expect_contains(emulate_reply, R"("race_name":"MortalRace")");
  test::expect_contains(emulate_reply, R"("governor_name":"MortalGov")");

  server.shutdown();
}

}  // namespace

int main() {
  test_server_initialization_and_registry_primitives();
  test_server_network_lifecycle_and_session_handling();
  test_server_quotas_idle_timeout_and_turn_events();
  test_server_slow_client_flush_reaping();
  test_server_deity_departure_broadcast();
  test_server_json_mode_single_session_stanza();

  std::println(std::cout, "✓ server_test passed!");
  return 0;
}
