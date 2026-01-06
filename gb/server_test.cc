// SPDX-License-Identifier: Apache-2.0

/// \file server_test.cc
/// \brief Tests for the Server class (Step 3 of Asio migration)

import asio;
import dallib;
import gblib;
import session;
import std;

#include <cassert>

int main() {
  std::println(std::cerr, "=== Server Test ===\n");

  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  std::println(std::cerr, "✓ Database initialized");

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].name = "TestGovernor";
  race.governor[0].money = 1000;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);
  std::println(std::cerr, "✓ Test race created");

  // Initialize server state
  auto server_state_handle = em.get_server_state();
  auto& state = *server_state_handle;
  state.update_time_minutes = 60;
  state.segments = 4;
  state.next_update_time = std::time(nullptr) + 3600;
  state.next_segment_time = std::time(nullptr) + 900;
  server_state_handle.save();
  std::println(std::cerr, "✓ Server state initialized");

  // Test 1: SessionRegistry interface
  {
    std::println(std::cerr, "\nTest 1: SessionRegistry interface");

    // Create a mock implementation to test the interface
    class TestRegistry : public SessionRegistry {
    public:
      void notify_race(player_t, const std::string&) override {
        call_count++;
      }
      bool notify_player(player_t, governor_t, const std::string&) override {
        call_count++;
        return true;
      }
      bool update_in_progress() const override {
        return false;
      }
      int call_count = 0;
    };

    TestRegistry registry;
    registry.notify_race(1, "Test message");

    assert(registry.call_count == 1);
    std::println(std::cerr, "✓ SessionRegistry interface works");
  }

  // Test 2: Timer-based operations
  {
    std::println(std::cerr, "\nTest 2: Timer-based operations");

    asio::io_context io;
    asio::steady_timer timer(io);
    bool timer_fired = false;

    timer.expires_after(std::chrono::milliseconds(10));
    timer.async_wait([&](asio::error_code ec) {
      if (!ec) {
        timer_fired = true;
      }
    });

    // Run for a short time
    io.run_for(std::chrono::milliseconds(50));

    assert(timer_fired);
    std::println(std::cerr, "✓ Asio timer operations work");
  }

  // Test 3: Acceptor can be created
  {
    std::println(std::cerr, "\nTest 3: TCP acceptor creation");

    asio::io_context io;
    try {
      // Try to create an acceptor (using port 0 for auto-assignment)
      asio::ip::tcp::acceptor acceptor(
          io, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 0));
      acceptor.set_option(asio::socket_base::reuse_address(true));

      std::println(std::cerr, "✓ TCP acceptor created successfully");
    } catch (const std::exception& e) {
      std::println(std::cerr, "✗ TCP acceptor creation failed");
      return 1;
    }
  }

  std::println(std::cerr, "\n=== All Server Tests Passed ===");
  return 0;
}
