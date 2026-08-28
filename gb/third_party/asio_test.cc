// SPDX-License-Identifier: Apache-2.0

/// \file asio_test.cc
/// \brief Unit test validating asio io_context event loop and steady_timer
/// async callbacks.

import asio;
import test;
import std;

#include <cstdio>

int main() {
  // io_context creation
  asio::io_context io;
  test::expect_false(io.stopped());

  // Timer with async_wait
  asio::steady_timer timer(io);
  timer.expires_after(std::chrono::milliseconds(10));

  bool called = false;
  timer.async_wait([&](asio::error_code ec) {
    called = true;
    test::expect_false(static_cast<bool>(ec));
  });

  // Run the event loop - should execute the timer callback
  io.run();
  test::expect_true(called);

  // Verify io_context stopped after all work complete
  test::expect_true(io.stopped());

  std::println(std::cout, "✓ Asio module wrapper test passed!");
  return 0;
}
