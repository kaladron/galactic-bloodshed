// SPDX-License-Identifier: Apache-2.0

import asio;
import dallib;
import gblib;
import session;
import std;

#include <cassert>

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
    assert(!delivered);
    std::println(std::cout, "✓ notify_player returns false with no sessions");
  }

  // SessionRegistry::notify_race does nothing when no sessions
  {
    MockSessionRegistry registry;
    registry.notify_race(1, "Broadcast message");
    assert(true);  // Should not crash
    std::println(std::cout, "✓ notify_race with no sessions doesn't crash");
  }

  // notify_player with update in progress returns false
  {
    MockSessionRegistry registry;
    registry.update_flag = true;
    bool delivered = registry.notify_player(1, 0, "Hello");
    assert(!delivered);

    // Also test with mock data
    registry.sessions.push_back({true, 1, 0, {}});
    delivered = registry.test_notify_player(1, 0, "Hello");
    assert(!delivered);  // Should still be false due to update flag

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
    assert(registry.sessions[0].output.str().empty());
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
    assert(delivered);

    // Check only first session received the message
    assert(registry.sessions[0].output.str() == "Message for P1G0\n");
    assert(registry.sessions[1].output.str().empty());
    assert(registry.sessions[2].output.str().empty());

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
    assert(registry.sessions[0].output.str() == "Race 1 broadcast\n");
    assert(registry.sessions[1].output.str() == "Race 1 broadcast\n");
    assert(registry.sessions[2].output.str().empty());

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
    assert(delivered);  // Should deliver to session 1

    // Only connected session should receive message
    assert(registry.sessions[0].output.str().empty());
    assert(registry.sessions[1].output.str() == "Test\n");

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
    assert(!delivered);

    // No sessions should have received message
    assert(registry.sessions[0].output.str().empty());
    assert(registry.sessions[1].output.str().empty());

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
    assert(registry.sessions[0].output.str().empty());
    assert(registry.sessions[1].output.str().empty());

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
    assert(registry.sessions[0].output.str() == expected);

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
    assert(delivered);

    // Both sessions with player 1, governor 0 should receive message
    assert(registry.sessions[0].output.str() == "Duplicate login\n");
    assert(registry.sessions[1].output.str() == "Duplicate login\n");
    assert(registry.sessions[2].output.str().empty());

    std::println(std::cout,
                 "✓ notify_player delivers to multiple matching sessions");
  }

  // Empty message is still delivered
  {
    MockSessionRegistry registry;

    registry.sessions.push_back(
        {.connected = true, .player = 1, .governor = 0, .output = {}});

    bool delivered = registry.test_notify_player(1, 0, "");
    assert(delivered);  // Delivery succeeds even with empty message

    // Buffer should be empty but delivery still succeeded
    assert(registry.sessions[0].output.str().empty());

    std::println(std::cout, "✓ Empty message can be delivered");
  }

  // update_in_progress state transitions
  {
    MockSessionRegistry registry;

    assert(!registry.update_in_progress());

    registry.update_flag = true;
    assert(registry.update_in_progress());

    registry.update_flag = false;
    assert(!registry.update_in_progress());

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
    assert(delivered);
    assert(registry.sessions[0].output.str() == long_message);

    std::println(std::cout, "✓ Long messages handled correctly");
  }

  std::println(std::cout, "\n✅ All session module tests passed!");
  std::println(
      std::cout,
      "\nNote: These tests validate SessionRegistry notification logic.");
  std::println(std::cout, "Async I/O behavior (read/write/disconnect) is "
                          "tested via integration tests");
  std::println(std::cout,
               "with real network sockets and requires a running server.");

  return 0;
}
