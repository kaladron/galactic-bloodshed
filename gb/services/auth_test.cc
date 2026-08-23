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

#include <cassert>

namespace {

void test_make_command_t() {
  assert(make_command_t("").empty());
  assert(make_command_t("   ").empty());

  auto res1 = make_command_t("hello world");
  assert(res1.size() == 2);
  assert(res1[0] == "hello" && res1[1] == "world");

  auto res2 = make_command_t("  spaced   out   args  ");
  assert(res2.size() == 3);
  assert(res2[0] == "spaced" && res2[1] == "out" && res2[2] == "args");
}

void test_parse_connect() {
  auto p0 = parse_connect("");
  assert(p0.player.empty() && p0.governor.empty());

  auto p1 = parse_connect("single_word");
  assert(p1.player.empty() && p1.governor.empty());

  auto p2 = parse_connect("racepass govpass");
  assert(p2.player == "racepass");
  assert(p2.governor == "govpass");

  auto p3 = parse_connect("too many arguments passed");
  assert(p3.player.empty() && p3.governor.empty());
}

void test_welcome_user() {
  TestContext ctx;
  auto state_handle = ctx.em.get_server_state();
  state_handle->welcome_message = "Custom Welcome MotD\n";
  state_handle.save();

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
  assert(output.contains("Welcome to Galactic Bloodshed"));
  assert(output.contains("Custom Welcome MotD"));
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
  assert(!session1->connected());

  // 2. Non-existent credentials
  asio::ip::tcp::socket socket2(io);
  auto session2 = std::make_shared<Session>(std::move(socket2), ctx.em,
                                            registry, [](auto) {});
  check_connect(*session2, "wrong password");
  assert(!session2->connected());
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session2->out());
  assert(out_stream.str().contains("Connection refused."));
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

  // Mock registry that reports (1, 0) as already connected
  class BusyRegistry : public NullSessionRegistry {
  public:
    bool is_connected(player_t p, governor_t g) const override {
      return p == 1 && g == 0;
    }
  } busy_registry;

  asio::io_context io;
  asio::ip::tcp::socket socket(io);
  auto session = std::make_shared<Session>(std::move(socket), ctx.em,
                                           busy_registry, [](auto) {});

  check_connect(*session, "raceword govword");
  assert(!session->connected());
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session->out());
  assert(out_stream.str().contains("Connection refused."));
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

  assert(session->connected());
  assert(session->player() == 1);
  assert(session->governor() == 0);
  assert(session->snum() == 0);  // Clamped from 999
  assert(session->pnum() == 0);  // Clamped from 999

  // Verify race login time updated in database
  const auto* updated_race = ctx.em.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->governor[0].login > 0);

  // Verify login output
  std::ostringstream& out_stream =
      static_cast<std::ostringstream&>(session->out());
  std::string output = out_stream.str();
  assert(output.contains("TestRace \"Gov0\" [1,0] logged on."));
  assert(output.contains("Government Center #42 is active."));
  assert(output.contains("Morale: 100"));
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
