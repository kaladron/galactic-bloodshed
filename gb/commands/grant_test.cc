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

  // Create test race with multiple governors via repository
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.governor[0].money = 1000;
  race.governor[1].active = true;
  race.governor[1].money = 500;
  race.governor[1].name = "SubGov";

  // Save race via repository
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);  // Set race pointer like production
  g.level = ScopeLevel::LEVEL_UNIV;

  std::println("Test 1: Grant money to governor");
  {
    command_t argv = {"grant", "1", "money", "200"};
    GB::commands::grant(argv, g);
    
    // Verify money was transferred
    const auto* saved_race = em.peek_race(1);
    assert(saved_race != nullptr);
    assert(saved_race->governor[0].money == 800);  // 1000 - 200
    assert(saved_race->governor[1].money == 700);  // 500 + 200
    std::println("    ✓ Money granted: gov[0]={}, gov[1]={}",
                 saved_race->governor[0].money,
                 saved_race->governor[1].money);
  }

  std::println("\n✅ All grant tests passed!");
  return 0;
}
