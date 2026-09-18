// SPDX-License-Identifier: Apache-2.0

/// \file session_test.cc
/// \brief Unit tests for SessionRegistry notification routing, disconnection
/// handling, and update suppression.

import asio;
import dallib;
import gb.entities;
import gb.services;
import gb.server;
import session;
import test;
import std;

// Mock Session for testing without real sockets
// This allows us to test SessionRegistry logic without async I/O
class MockSession : public Session {
public:
  // We can't construct a real Session without a connected socket,
  // so we'll test SessionRegistry methods with a different approach
};

// Mock SessionRegistry for testing with mock session data
class MockSessionRegistry : public SessionRegistry {
public:
  struct SessionData {
    bool connected;
    player_t player;
    governor_t governor;
    std::ostringstream output;
  };

  std::vector<SessionData> sessions;
  bool update_flag = false;

  void notify_race(player_t race, const std::string& message) override {
    if (update_in_progress()) return;
    for (auto& session : sessions) {
      if (session.connected && session.player == race) {
        session.output << message;
      }
    }
  }

  bool notify_player(player_t race, governor_t gov,
                     const std::string& message) override {
    if (update_in_progress()) return false;
    bool delivered = false;
    for (auto& session : sessions) {
      if (session.connected && session.player == race &&
          session.governor == gov) {
        session.output << message;
        delivered = true;
      }
    }
    return delivered;
  }

  bool update_in_progress() const override {
    return update_flag;
  }

  // Aliases for the old test helper names (for minimal test changes)
  bool test_notify_player(player_t race, governor_t gov,
                          const std::string& message) {
    return notify_player(race, gov, message);
  }

  void test_notify_race(player_t race, const std::string& message) {
    notify_race(race, message);
  }
};

int main() {
  // Setup test database and entity manager
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Running session module tests...\n");

  // SessionRegistry::notify_player returns false when no sessions
  {
    MockSessionRegistry registry;
    bool delivered = registry.notify_player(1, 0, "Hello");
    test::expect_false(delivered);
    std::println(std::cout, "✓ notify_player returns false with no sessions");
  }

  // SessionRegistry::notify_race does nothing when no sessions
  {
    MockSessionRegistry registry;
    registry.notify_race(1, "Broadcast message");
    std::println(std::cout, "✓ notify_race with no sessions doesn't crash");
  }

  // notify_player with update in progress returns false
  {
    MockSessionRegistry registry;
    registry.update_flag = true;
    bool delivered = registry.notify_player(1, 0, "Hello");
    test::expect_false(delivered);

    // Also test with mock data
    registry.sessions.push_back({true, 1, 0, {}});
    delivered = registry.test_notify_player(1, 0, "Hello");
    test::expect_false(delivered);  // Should still be false due to update flag

    std::println(std::cout, "✓ notify_player returns false during update");
  }

  // notify_race during update does nothing
  {
    MockSessionRegistry registry;
    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});

    registry.update_flag = true;
    registry.test_notify_race(1, "Test message");

    // Message should not be in buffer since update is in progress
    test::expect_true(registry.sessions[0].output.str().empty());
    std::println(std::cout, "✓ notify_race suppressed during update");
  }

  // notify_player delivers to correct session
  {
    MockSessionRegistry registry;

    // Create multiple sessions
    registry.sessions.push_back({.connected = true,
                                 .player = 1,
                                 .governor = 0,
                                 .output = {}});  // Match
    registry.sessions.push_back({.connected = true,
                                 .player = 1,
                                 .governor = 1,
                                 .output = {}});  // Different governor
    registry.sessions.push_back({.connected = true,
                                 .player = 2,
                                 .governor = 0,
                                 .output = {}});  // Different player

    // Send to player 1, governor 0
    bool delivered = registry.test_notify_player(1, 0, "Message for P1G0\n");
    test::expect_true(delivered);

    // Check only first session received the message
    test::expect_eq(registry.sessions[0].output.str(), "Message for P1G0\n");
    test::expect_true(registry.sessions[1].output.str().empty());
    test::expect_true(registry.sessions[2].output.str().empty());

    std::println(std::cout, "✓ notify_player delivers to correct session");
  }

  // notify_race delivers to all governors of a race
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});
    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 1, .output = {}});
    registry.sessions.push_back(
        {.connected = true, .player = 2, .governor = 0, .output = {}});

    // Broadcast to race 1
    registry.test_notify_race(1, "Race 1 broadcast\n");

    // Check sessions 0 and 1 received message, but not session 2
    test::expect_eq(registry.sessions[0].output.str(), "Race 1 broadcast\n");
    test::expect_eq(registry.sessions[1].output.str(), "Race 1 broadcast\n");
    test::expect_true(registry.sessions[2].output.str().empty());

    std::println(std::cout, "✓ notify_race delivers to all governors of race");
  }

  // Disconnected sessions don't receive notifications
  {
    MockSessionRegistry registry;

    registry.sessions.push_back({.connected = false,
                                 .player = 1,
                                 .governor = 0,
                                 .output = {}});  // Disconnected
    registry.sessions.push_back({.connected = true,
                                 .player = 1,
                                 .governor = 0,
                                 .output = {}});  // Connected

    // Send to player 1, governor 0
    bool delivered = registry.test_notify_player(1, 0, "Test\n");
    test::expect_true(delivered);  // Should deliver to session 1

    // Only connected session should receive message
    test::expect_true(registry.sessions[0].output.str().empty());
    test::expect_eq(registry.sessions[1].output.str(), "Test\n");

    std::println(std::cout,
                 "✓ Disconnected sessions don't receive notifications");
  }

  // notify_player to wrong player/governor returns false
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});
    registry.sessions.push_back(
        {.connected = true, .player = 2, .governor = 1, .output = {}});

    // Try to send to player 3, governor 0 (no matching session)
    bool delivered = registry.test_notify_player(3, 0, "Test\n");
    test::expect_false(delivered);

    // No sessions should have received message
    test::expect_true(registry.sessions[0].output.str().empty());
    test::expect_true(registry.sessions[1].output.str().empty());

    std::println(std::cout,
                 "✓ notify_player to non-existent player returns false");
  }

  // notify_race to race with no sessions
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});
    registry.sessions.push_back(
        {.connected = true, .player = 2, .governor = 0, .output = {}});

    // Broadcast to race 3 (no sessions)
    registry.test_notify_race(3, "Nobody home\n");

    // No sessions should have received message
    test::expect_true(registry.sessions[0].output.str().empty());
    test::expect_true(registry.sessions[1].output.str().empty());

    std::println(std::cout,
                 "✓ notify_race to race with no sessions (no crash)");
  }

  // Multiple messages to same session accumulate
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});

    // Send multiple messages
    registry.test_notify_player(1, 0, "Message 1\n");
    registry.test_notify_player(1, 0, "Message 2\n");
    registry.test_notify_race(1, "Broadcast\n");

    // All messages should be in the buffer
    std::string expected = "Message 1\nMessage 2\nBroadcast\n";
    test::expect_eq(registry.sessions[0].output.str(), expected);

    std::println(std::cout, "✓ Multiple messages accumulate in output buffer");
  }

  // notify_player can deliver to multiple matching sessions
  // (same player/governor logged in multiple times)
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});
    registry.sessions.push_back({.connected = true,
                                 .player = 1,
                                 .governor = 0,
                                 .output = {}});  // Same player/governor
    registry.sessions.push_back(
        {.connected = true, .player = 2, .governor = 0, .output = {}});

    bool delivered = registry.test_notify_player(1, 0, "Duplicate login\n");
    test::expect_true(delivered);

    // Both sessions with player 1, governor 0 should receive message
    test::expect_eq(registry.sessions[0].output.str(), "Duplicate login\n");
    test::expect_eq(registry.sessions[1].output.str(), "Duplicate login\n");
    test::expect_true(registry.sessions[2].output.str().empty());

    std::println(std::cout,
                 "✓ notify_player delivers to multiple matching sessions");
  }

  // Empty message is still delivered
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});

    bool delivered = registry.test_notify_player(1, 0, "");
    test::expect_true(delivered);  // Delivery succeeds even with empty message

    // Buffer should be empty but delivery still succeeded
    test::expect_true(registry.sessions[0].output.str().empty());

    std::println(std::cout, "✓ Empty message can be delivered");
  }

  // update_in_progress state transitions
  {
    MockSessionRegistry registry;

    test::expect_false(registry.update_in_progress());

    registry.update_flag = true;
    test::expect_true(registry.update_in_progress());

    registry.update_flag = false;
    test::expect_false(registry.update_in_progress());

    std::println(std::cout, "✓ update_in_progress state management");
  }

  // Long messages are handled correctly
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});

    std::string long_message(1000, 'X');
    long_message += "\n";

    bool delivered = registry.test_notify_player(1, 0, long_message);
    test::expect_true(delivered);
    test::expect_eq(registry.sessions[0].output.str(), long_message);

    std::println(std::cout, "✓ Long messages handled correctly");
  }

  // Real Session state accessors, quota rate-limiting, and async I/O
  {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor(
        io, asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), 0));
    asio::ip::tcp::socket client_socket(io);
    client_socket.connect(acceptor.local_endpoint());
    asio::ip::tcp::socket server_socket = acceptor.accept();

    MockSessionRegistry registry;
    bool disconnected = false;
    auto session = std::make_shared<Session>(
        std::move(server_socket), em, registry,
        [&disconnected](std::shared_ptr<Session>) { disconnected = true; });

    // Initial state
    test::expect_false(session->connected());
    test::expect_eq(session->player().value, 0);
    test::expect_eq(session->governor().value, 0);
    test::expect_false(session->god());
    test::expect_eq(session->snum(), 0);
    test::expect_eq(session->pnum(), 0);
    test::expect_eq(session->shipno(), 0);
    test::expect_true(session->level() == ScopeLevel::LEVEL_UNIV);
    test::expect_eq(session->quota(), COMMAND_BURST_SIZE);
    test::expect_eq(session->last_time(), 0);
    test::expect_false(session->has_pending_input());
    test::expect_eq(session->pop_input(), "");
    test::expect_false(session->has_pending_output());
    test::expect_eq(session->write_queue_size(), 0u);
    test::expect_true(&session->entity_manager() == &em);
    test::expect_true(&session->registry() == &registry);

    // State mutations
    session->set_connected(true);
    session->set_player(2);
    session->set_governor(3);
    session->set_god(true);
    session->set_snum(4);
    session->set_pnum(5);
    session->set_shipno(6);
    session->set_level(ScopeLevel::LEVEL_PLAN);
    session->touch();

    test::expect_true(session->connected());
    test::expect_eq(session->player().value, 2);
    test::expect_eq(session->governor().value, 3);
    test::expect_true(session->god());
    test::expect_eq(session->snum(), 4);
    test::expect_eq(session->pnum(), 5);
    test::expect_eq(session->shipno(), 6);
    test::expect_true(session->level() == ScopeLevel::LEVEL_PLAN);
    test::expect_true(session->last_time() > 0);

    // Rate limiting quota mechanics
    session->use_quota();
    test::expect_eq(session->quota(), COMMAND_BURST_SIZE - 1);
    session->add_quota(10);
    test::expect_eq(session->quota(), COMMAND_BURST_SIZE);
    for (int i = 0; i < COMMAND_BURST_SIZE + 5; ++i) {
      session->use_quota();
    }
    test::expect_eq(session->quota(), 0);
    session->add_quota(5);
    test::expect_eq(session->quota(), 5);

    // Async input reading (\n, \r\n, and empty line filtering)
    session->start();
    std::string raw_input = "first_cmd\nsecond_cmd\r\n\n";
    client_socket.write_some(asio::buffer(raw_input));
    io.poll();

    test::expect_true(session->has_pending_input());
    test::expect_eq(session->pop_input(), "first_cmd");
    test::expect_true(session->has_pending_input());
    test::expect_eq(session->pop_input(), "second_cmd");
    test::expect_false(session->has_pending_input());

    // Output buffering and network flush
    session->out() << "Server reply line\n";
    test::expect_true(session->has_pending_output());
    session->flush_to_network();
    test::expect_false(session->has_pending_output());
    io.poll();

    std::array<char, 128> read_buf{};
    std::size_t bytes = client_socket.read_some(asio::buffer(read_buf));
    test::expect_eq(std::string(read_buf.data(), bytes), "Server reply line\n");

    // Graceful disconnect
    session->disconnect();
    test::expect_true(disconnected);

    std::println(
        std::cout,
        "✓ Real Session state accessors, quotas, and async I/O verified");
  }

  // Input flooding overflow disconnects session
  {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor(
        io, asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), 0));
    asio::ip::tcp::socket client_socket(io);
    client_socket.connect(acceptor.local_endpoint());
    asio::ip::tcp::socket server_socket = acceptor.accept();

    MockSessionRegistry registry;
    bool flood_disconnected = false;
    auto session = std::make_shared<Session>(
        std::move(server_socket), em, registry,
        [&flood_disconnected](std::shared_ptr<Session>) {
          flood_disconnected = true;
        });

    session->start();
    std::string flood_payload;
    for (std::size_t i = 0; i <= MAX_INPUT_QUEUE_SIZE; ++i) {
      flood_payload += "cmd\n";
    }
    client_socket.write_some(asio::buffer(flood_payload));
    io.poll();

    test::expect_true(flood_disconnected);
    std::println(std::cout,
                 "✓ Input queue overflow disconnects flooding client");
  }

  // Oversized command line (> MAX_COMMAND_LEN) disconnects session
  {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor(
        io, asio::ip::tcp::endpoint(asio::ip::address_v6::loopback(), 0));
    asio::ip::tcp::socket client_socket(io);
    client_socket.connect(acceptor.local_endpoint());
    asio::ip::tcp::socket server_socket = acceptor.accept();

    MockSessionRegistry registry;
    bool oversized_disconnected = false;
    auto session = std::make_shared<Session>(
        std::move(server_socket), em, registry,
        [&oversized_disconnected](std::shared_ptr<Session>) {
          oversized_disconnected = true;
        });

    session->start();
    std::string oversized_line(MAX_COMMAND_LEN + 10, 'A');
    oversized_line.push_back('\n');
    client_socket.write_some(asio::buffer(oversized_line));
    io.poll();

    test::expect_true(oversized_disconnected);
    test::expect_false(session->has_pending_input());
    std::println(std::cout,
                 "✓ Oversized command (> MAX_COMMAND_LEN) disconnects client");
  }

  std::println(std::cout, "\n✅ All session module tests passed!");
  return 0;
}
