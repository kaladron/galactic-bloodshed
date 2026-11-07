// SPDX-License-Identifier: Apache-2.0

/// \file auth_test.cc
/// \brief Unit tests for password parsing, authentication, and login handshake.

import asio;
import auth;
import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.server;
import session;
import test;
import std;

namespace {

void test_make_command_t() {
  test::expect_true(make_command_t("").empty());
  test::expect_true(make_command_t("   ").empty());

  auto res1 = make_command_t("hello world");
  test::expect_eq(res1.size(), 2);
  test::expect_eq(res1[0], "hello");
  test::expect_eq(res1[1], "world");

  auto res2 = make_command_t("  spaced   out   args  ");
  test::expect_eq(res2.size(), 3);
  test::expect_eq(res2[0], "spaced");
  test::expect_eq(res2[1], "out");
  test::expect_eq(res2[2], "args");
}

void test_parse_connect() {
  auto p0 = parse_connect("");
  test::expect_true(p0.player.empty() && p0.governor.empty());

  auto p1 = parse_connect("single_word");
  test::expect_true(p1.player.empty() && p1.governor.empty());

  auto p2 = parse_connect("racepass govpass");
  test::expect_eq(p2.player, "racepass");
  test::expect_eq(p2.governor, "govpass");
  test::expect_true(p2.ui_mode == UiMode::ASCII);

  auto p_json = parse_connect("racepass govpass json");
  test::expect_eq(p_json.player, "racepass");
  test::expect_eq(p_json.governor, "govpass");
  test::expect_true(p_json.ui_mode == UiMode::JSON);

  auto p3 = parse_connect("too many arguments passed");
  test::expect_true(p3.player.empty() && p3.governor.empty());
}

void test_welcome_user() {
  TestContext ctx;
  ctx.em.mutate_server_state(
      [](ServerState& s) { s.welcome_message = "Custom Welcome MotD\n"; });

  asio::io_context io;
  asio::ip::tcp::acceptor acceptor(
      io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
  asio::ip::tcp::socket client_sock(io);
  client_sock.connect(acceptor.local_endpoint());
  asio::ip::tcp::socket server_sock = acceptor.accept();

  auto& registry = get_test_session_registry();
  auto session = Session::create(std::move(server_sock), ctx.em, registry);

  welcome_user(*session, ctx.em);
  io.poll();

  std::array<char, 512> buf{};
  std::size_t len = client_sock.read_some(asio::buffer(buf));
  std::string output(buf.data(), len);
  test::expect_contains(
      output, std::format("Welcome to Galactic Bloodshed v{}", GB_VERSION));
  test::expect_contains(output, "Custom Welcome MotD");
}

void test_check_connect_failure() {
  TestContext ctx;
  asio::io_context io;
  asio::ip::tcp::socket socket1(io);
  auto& registry = get_test_session_registry();
  auto session1 = Session::create(std::move(socket1), ctx.em, registry);

  // 1. Invalid argument count
  check_connect(*session1, "only_one");
  test::expect_false(session1->connected());

  // 2. Non-existent credentials
  asio::ip::tcp::socket socket2(io);
  auto session2 = Session::create(std::move(socket2), ctx.em, registry);
  check_connect(*session2, "wrong password");
  test::expect_false(session2->connected());
  test::expect_contains(session2->pending_output(), "Connection refused.");
}

void test_check_connect_duplicate_session_rejection() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& race) {
    race.name = "TestRace";
    race.password = "raceword";
    race.leader().name = "Gov1";
    race.leader().password = "govword";
  });

  // Use RecordingSessionRegistry configured with active session for player 1
  // governor 1
  RecordingSessionRegistry busy_registry;
  busy_registry.sessions = {
      SessionInfo{.player = 1, .governor = 1, .connected = true},
  };

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto session = Session::create(std::move(socket), ctx.em, busy_registry);

  check_connect(*session, "raceword govword");
  test::expect_false(session->connected());
  test::expect_contains(session->pending_output(), "Connection refused.");

  ctx.verify_universe_invariants();
}

void test_check_connect_success_and_clamping() {
  TestContext ctx;
  ctx.with_standard_universe();

  const auto gov_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                          .owned_by(1, 1)
                          .landed_on(1, 1, {0, 0})
                          .build();

  ctx.em.mutate_race(1, [&](Race& race) {
    race.name = "TestRace";
    race.password = "raceword";
    race.morale = 100;
    race.Gov_ship = gov_id;
    race.leader().name = "Gov1";
    race.leader().password = "govword";
    race.leader().deflevel = ScopeLevel::LEVEL_PLAN;
    race.leader().defsystem = 999;     // Out of bounds -> should clamp to 1
    race.leader().defplanetnum = 999;  // Out of bounds -> should clamp to 1
    race.leader().login = 1000000;
  });

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto& registry = get_test_session_registry();
  auto session = Session::create(std::move(socket), ctx.em, registry);

  check_connect(*session, "raceword govword");

  test::expect_true(session->connected());
  test::expect_eq(session->player(), player_t{1});
  test::expect_eq(session->governor(), governor_t{1});
  test::expect_eq(session->snum(), starnum_t{1});    // Clamped from 999
  test::expect_eq(session->pnum(), planetnum_t{1});  // Clamped from 999

  // Verify race login time updated in database
  const auto* updated_race = ctx.em.peek_race(1);
  test::expect_true(updated_race != nullptr);
  test::expect_gt(updated_race->leader().login, 1000000);

  // Verify login output
  std::string_view output = session->pending_output();
  test::expect_contains(output, "TestRace \"Gov1\" [1,1] logged on.");
  test::expect_contains(
      output, std::format("Government Center #{} is active.", gov_id));
  test::expect_contains(
      output, std::format("Last login      : {}", format_timestamp(1000000)));
  test::expect_contains(output, "Morale: 100");
  test::expect_contains(output, "You have:");

  ctx.verify_universe_invariants();
}

void test_check_connect_json_mode() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& race) {
    race.name = "JsonRace";
    race.password = "raceword";
    race.morale = 85;
    race.leader().name = "JsonGov";
    race.leader().password = "govword";
    race.leader().login = 1000000;
  });

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto& registry = get_test_session_registry();
  auto session = Session::create(std::move(socket), ctx.em, registry);

  check_connect(*session, "raceword govword json");
  test::expect_true(session->connected());
  test::expect_true(session->ui_mode() == UiMode::JSON);

  // Verify race login time updated in database
  const auto* updated_race = ctx.em.peek_race(1);
  test::expect_true(updated_race != nullptr);
  const std::time_t updated_login = updated_race->leader().login;
  test::expect_gt(updated_login, 1000000);

  // Also send a plain-text notification to verify Session::send text wrapping
  session->send("Broadcast alert\n");

  std::string_view output = session->pending_output();
  test::expect_contains(output, R"("type":"session")");
  test::expect_contains(output, R"("race_name":"JsonRace")");
  test::expect_contains(output, R"("governor_name":"JsonGov")");
  test::expect_contains(output, R"("type":"race")");
  test::expect_contains(output, R"("morale":85)");
  test::expect_contains(output, R"("money":10000)");
  test::expect_false(output.contains(R"("last_login":1000000)"));
  test::expect_contains(output,
                        std::format(R"("last_login":{})", updated_login));
  test::expect_contains(output, R"("type":"message")");
  test::expect_contains(output, R"("message":"Broadcast alert")");

  ctx.verify_universe_invariants();
}

void test_check_connect_deity_arrival_broadcast() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& race) {
    race.name = "GodRace";
    race.password = "godword";
    race.God = true;
    race.leader().name = "Almighty";
    race.leader().password = "govword";
  });

  ctx.em.mutate_race(2, [](Race& race) {
    race.name = "MortalRace";
    race.password = "mortalword";
    race.God = false;
    race.leader().name = "Peasant";
    race.leader().password = "govword";
  });

  // Active session for mortal Player 2 in RecordingSessionRegistry
  RecordingSessionRegistry registry;
  registry.sessions = {
      SessionInfo{.player = 2, .governor = 1, .connected = true},
  };

  asio::io_context io;

  // 1. Deity (Race 1, God = true) logs in: mortal receives arrival broadcast
  {
    asio::ip::tcp::socket socket1(io);
    auto session1 = Session::create(std::move(socket1), ctx.em, registry);
    check_connect(*session1, "godword govword");
    test::expect_true(session1->connected());
    test::expect_true(session1->god());
    test::expect_true(
        registry.has_received(player_t{2}, "has entered the universe"));
    test::expect_true(registry.has_received(
        player_t{2},
        "*** Deity GodRace \"Almighty\" [1,1] has entered the universe. ***"));
  }

  // 2. Mortal (Race 2, God = false) logs in: player 1 does NOT receive arrival
  // broadcast
  registry.clear_notifications();
  {
    // Clear active session for Player 2 so login is permitted
    registry.sessions = {
        SessionInfo{.player = 1, .governor = 1, .connected = true},
    };
    asio::ip::tcp::socket socket2(io);
    auto session2 = Session::create(std::move(socket2), ctx.em, registry);
    check_connect(*session2, "mortalword govword");
    test::expect_true(session2->connected());
    test::expect_false(session2->god());
    test::expect_false(
        registry.has_received(player_t{1}, "has entered the universe"));
  }

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_make_command_t();
  test_parse_connect();
  test_welcome_user();
  test_check_connect_failure();
  test_check_connect_duplicate_session_rejection();
  test_check_connect_success_and_clamping();
  test_check_connect_json_mode();
  test_check_connect_deity_arrival_broadcast();

  std::println(std::cout, "✓ auth_test passed!");
  return 0;
}
