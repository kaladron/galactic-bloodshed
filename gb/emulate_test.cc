// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // NOTE: This is a minimal test that verifies prerequisites for the emulate
  // command. The emulate command requires Session& which needs async I/O
  // infrastructure (asio::io_context, sockets, ability to modify session
  // state).
  //
  // TODO(Step 9): After GameObj can access Session, convert emulate to
  // (const command_t&, GameObj&) signature and add proper unit tests.
  //
  // For now, this test verifies:
  // 1. Race data with multiple governors can be loaded (prerequisite)
  // 2. The command compiles and links correctly
  //
  // Proper testing requires:
  // - MockSession class with set_player(), set_governor(), set_god() methods
  // - Calling GB::commands::emulate(argv, mock_session)
  // - Verifying session state changes and output messages

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create test races with multiple governors
  Race race1{};
  race1.Playernum = 1;
  race1.name = "TestRace1";
  race1.Guest = false;
  race1.God = false;
  race1.governor[0].active = true;
  race1.governor[0].name = "Governor0";
  race1.governor[1].active = true;
  race1.governor[1].name = "Governor1";

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);

  // Verify both governors exist (prerequisite for emulate command)
  const auto* saved = em.peek_race(1);
  assert(saved);
  assert(saved->governor[0].active);
  assert(saved->governor[1].active);
  assert(saved->governor[0].name == "Governor0");
  assert(saved->governor[1].name == "Governor1");

  std::println("EMULATE prerequisite test passed!");
  std::println(
      "Note: Full command testing requires MockSession implementation.");
  return 0;
}
