// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import session;
import std;

#include <cassert>

// Mock SessionRegistry for testing
class MockSessionRegistry : public SessionRegistry {
public:
  std::vector<std::pair<player_t, std::string>> messages;

  void for_each_session(std::function<void(Session&)>) override {
    // No sessions in mock - tests notification logic only
  }

  bool update_in_progress() const override {
    return false;
  }
};

int main() {
  // Test 1: SessionRegistry::notify_player returns false when no sessions
  MockSessionRegistry registry;
  bool delivered = registry.notify_player(1, 0, "Hello");
  assert(!delivered);

  // Test 2: SessionRegistry::notify_race does nothing when no sessions
  registry.notify_race(1, "Broadcast message");
  assert(true);  // Should not crash

  std::println(std::cerr, "Session module test passed!");
  return 0;
}
