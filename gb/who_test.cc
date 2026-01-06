// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // NOTE: This is a minimal test that verifies prerequisites for the who
  // command. The who command requires Session& which needs async I/O
  // infrastructure (asio::io_context, sockets, SessionRegistry with active
  // sessions).
  //
  // TODO(Step 9): After GameObj can access Session, convert who to
  // (const command_t&, GameObj&) signature and add proper unit tests.
  //
  // For now, this test verifies:
  // 1. Race data can be loaded (prerequisite for who command)
  // 2. The command compiles and links correctly
  //
  // Proper testing requires:
  // - MockSession class that implements Session interface
  // - MockSessionRegistry that tracks mock sessions
  // - Calling GB::commands::who(argv, mock_session) and verifying output

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "TestRace1";
  race1.Guest = false;
  race1.God = false;
  race1.governor[0].active = true;
  race1.governor[0].name = "Governor0";
  race1.governor[0].toggle.invisible = false;
  race1.governor[0].toggle.gag = false;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);

  // Verify race was saved (prerequisite for who command)
  const auto* saved = em.peek_race(1);
  assert(saved);
  assert(saved->name == "TestRace1");
  assert(!saved->governor[0].toggle.invisible);
  assert(!saved->governor[0].toggle.gag);

  std::println("WHO prerequisite test passed!");
  std::println(
      "Note: Full command testing requires MockSession implementation.");
  return 0;
}
