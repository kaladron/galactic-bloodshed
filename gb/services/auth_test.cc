// SPDX-License-Identifier: Apache-2.0

/// \file auth_test.cc
/// \brief Unit tests for password parsing, authentication, and login handshake.

import asio;
import auth;
import commands;
import dallib;
import gblib;
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
  auto session = std::make_shared<Session>(std::move(server_sock), ctx.em,
                                           registry, [](auto) {});

  welcome_user(*session, ctx.em);
  io.poll();

  std::array<char, 512> buf{};
  std::size_t len = client_sock.read_some(asio::buffer(buf));
  std::string output(buf.data(), len);
  test::expect_contains(output, "Welcome to Galactic Bloodshed");
  test::expect_contains(output, "Custom Welcome MotD");
}

void test_check_connect_failure() {
  TestContext ctx;
  asio::io_context io;
  asio::ip::tcp::socket socket1(io);
  auto& registry = get_test_session_registry();
  auto session1 = std::make_shared<Session>(std::move(socket1), ctx.em,
                                            registry, [](auto) {});

  // 1. Invalid argument count
  check_connect(*session1, "only_one");
  test::expect_false(session1->connected());

  // 2. Non-existent credentials
  asio::ip::tcp::socket socket2(io);
  auto session2 = std::make_shared<Session>(std::move(socket2), ctx.em,
                                            registry, [](auto) {});
  check_connect(*session2, "wrong password");
  test::expect_false(session2->connected());
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session2->out());
  test::expect_contains(out_stream.str(), "Connection refused.");
}

void test_check_connect_duplicate_session_rejection() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.password = "raceword";
  race.governor[0].name = "Gov0";
  race.governor[0].password = "govword";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  // Use RecordingSessionRegistry configured with active session for player 1
  // governor 0
  RecordingSessionRegistry busy_registry;
  busy_registry.sessions = {
      SessionInfo{.player = 1, .governor = 0, .connected = true},
  };

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto session = std::make_shared<Session>(std::move(socket), ctx.em,
                                           busy_registry, [](auto) {});

  check_connect(*session, "raceword govword");
  test::expect_false(session->connected());
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session->out());
  test::expect_contains(out_stream.str(), "Connection refused.");
}

void test_check_connect_success_and_clamping() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.password = "raceword";
  race.morale = 100;
  race.Gov_ship = 42;
  race.governor[0].name = "Gov0";
  race.governor[0].password = "govword";
  race.governor[0].deflevel = ScopeLevel::LEVEL_PLAN;
  race.governor[0].defsystem = 999;     // Out of bounds -> should clamp to 0
  race.governor[0].defplanetnum = 999;  // Out of bounds -> should clamp to 0
  race.governor[0].login = 0;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);

    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 0;
    sdata.name = "FirstStar";
    sdata.pnames = {"FirstPlanet"};
    Star star{sdata};
    star_repo.save(star);

    UniverseRepository univ_repo(store);
    universe_struct u{};
    u.id = 1;
    u.numstars = 1;
    univ_repo.save(u);
  }

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto& registry = get_test_session_registry();
  auto session = std::make_shared<Session>(std::move(socket), ctx.em, registry,
                                           [](auto) {});

  check_connect(*session, "raceword govword");

  test::expect_true(session->connected());
  test::expect_eq(session->player(), player_t{1});
  test::expect_eq(session->governor(), governor_t{0});
  test::expect_eq(session->snum(), starnum_t{0});    // Clamped from 999
  test::expect_eq(session->pnum(), planetnum_t{0});  // Clamped from 999

  // Verify race login time updated in database
  const auto* updated_race = ctx.em.peek_race(1);
  test::expect_true(updated_race != nullptr);
  test::expect_gt(updated_race->governor[0].login, 0);

  // Verify login output
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session->out());
  std::string output = out_stream.str();
  test::expect_contains(output, "TestRace \"Gov0\" [1,0] logged on.");
  test::expect_contains(output, "Government Center #42 is active.");
  test::expect_contains(output, "Morale: 100");
}

}  // namespace

int main() {
  test_make_command_t();
  test_parse_connect();
  test_welcome_user();
  test_check_connect_failure();
  test_check_connect_duplicate_session_rejection();
  test_check_connect_success_and_clamping();

  std::println(std::cout, "✓ auth_test passed!");
  return 0;
}
