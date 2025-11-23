// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create EntityManager
  EntityManager em(db);

  // Create two test races via repository
  Race race1{};
  race1.Playernum = 1;
  race1.name = "TestRace1";
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "TestRace2";
  race2.governor[0].active = true;

  // Save races via repository
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup universe_struct with AP points
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.AP[0] = 10;  // Give player 1 some AP points
  sdata.numstars = 0;
  universe_repo.save(sdata);

  // Initialize global Sdata
  getsdata(&Sdata);

  // Setup Num_races global
  Num_races = 2;

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.level = ScopeLevel::LEVEL_UNIV;

  std::println("Test 1: Declare alliance");
  {
    command_t argv = {"declare", "2", "alliance"};
    GB::commands::declare(argv, g);

    // Verify alliance was set
    const auto* saved_race1 = em.peek_race(1);
    const auto* saved_race2 = em.peek_race(2);
    assert(saved_race1 != nullptr);
    assert(saved_race2 != nullptr);
    assert(isset(saved_race1->allied, 2U));
    assert(!isset(saved_race1->atwar, 2U));
    // Verify translation was increased
    assert(saved_race2->translate[0] >= 30);
    std::println("    ✓ Alliance declared and translation updated");
  }

  std::println("\nTest 2: Declare war");
  {
    command_t argv = {"declare", "2", "war"};
    GB::commands::declare(argv, g);

    // Verify war was declared
    const auto* saved_race1 = em.peek_race(1);
    assert(saved_race1 != nullptr);
    assert(isset(saved_race1->atwar, 2U));
    assert(!isset(saved_race1->allied, 2U));
    std::println("    ✓ War declared successfully");
  }

  std::println("\nTest 3: Declare neutrality");
  {
    command_t argv = {"declare", "2", "neutrality"};
    GB::commands::declare(argv, g);

    // Verify neutrality was set
    const auto* saved_race1 = em.peek_race(1);
    assert(saved_race1 != nullptr);
    assert(!isset(saved_race1->atwar, 2U));
    assert(!isset(saved_race1->allied, 2U));
    std::println("    ✓ Neutrality declared successfully");
  }

  std::println("\n✅ All declare tests passed!");
  return 0;
}
