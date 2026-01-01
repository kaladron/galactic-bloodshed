// SPDX-License-Identifier: Apache-2.0

import std;
import dallib;
import gblib;
import session;
import notification;

#include <cassert>

// Mock SessionRegistry with no sessions
class EmptyRegistry : public SessionRegistry {
public:
  void for_each_session(std::function<void(Session&)>) override {}
  bool update_in_progress() const override {
    return false;
  }
};

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  EmptyRegistry registry;

  // Test: notify_player returns false when no matching session
  bool delivered = registry.notify_player(1, 0, "Hello");
  assert(!delivered);

  std::printf("Notification service test passed!\n");
  return 0;
}
