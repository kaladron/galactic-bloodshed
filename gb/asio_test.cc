// SPDX-License-Identifier: Apache-2.0

import asio;
import std;

#include <cassert>
#include <cstdio>

int main() {
  // Test 1: io_context creation
  asio::io_context io;
  assert(io.stopped() == false);

  // Test 2: Timer with async_wait
  asio::steady_timer timer(io);
  timer.expires_after(std::chrono::milliseconds(10));

  bool called = false;
  timer.async_wait([&](asio::error_code ec) {
    called = true;
    assert(!ec);
  });

  // Run the event loop - should execute the timer callback
  io.run();
  assert(called);

  // Test 3: Verify io_context stopped after all work complete
  assert(io.stopped() == true);

  std::println(stderr, "Asio module wrapper test passed!");
  return 0;
}
