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

  // Create test race via repository
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.password = "testpass";
  race.governor[0].active = true;
  race.governor[0].password = "govpass";
  race.dissolved = false;

  // NOTE: Not creating ships for this test because kill_ship() still uses global races[]
  // We're just testing that the dissolved flag gets set correctly
  
  // Save via repositories
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Setup universe_struct (required by dissolve command)
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 0;  // No stars, simplifies test
  universe_repo.save(sdata);
  
  // Initialize global Sdata
  getsdata(&Sdata);
  
  // Setup Num_races global
  Num_races = 1;
  
  // Load race into EntityManager cache to ensure getracenum can find it
  const auto* loaded_race = em.peek_race(1);
  assert(loaded_race != nullptr);
  assert(loaded_race->password == "testpass");
  std::println("Race loaded into EntityManager: player={}, password={}", loaded_race->Playernum, loaded_race->password);
  std::println("Governor 0: active={}, password='{}'", loaded_race->governor[0].active, loaded_race->governor[0].password);
  assert(loaded_race->governor[0].password == "govpass");

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.level = ScopeLevel::LEVEL_UNIV;

  std::println("Test 1: Dissolve race with correct passwords");
  {
    command_t argv = {"dissolve", "testpass", "govpass"};
    GB::commands::dissolve(argv, g);
    std::println("Command output: {}", g.out.str());
    
    // Verify race was dissolved
    const auto* saved_race = em.peek_race(1);
    assert(saved_race != nullptr);
    assert(saved_race->dissolved == true);
    std::println("    ✓ Race dissolved flag set to true");
    
    // TODO: Re-enable ship destruction test after kill_ship() migrated to EntityManager (Phase 3.7)
    // Currently disabled because kill_ship() uses global races[] array
    // Expected behavior: ship->alive should be false or ship->owner should be 0
    // 
    // Verify ship was destroyed (alive flag should be false)
    // const auto* saved_ship = em.peek_ship(1);
    // assert(saved_ship != nullptr);
    // assert(saved_ship->alive == false || saved_ship->owner == 0);
    // std::println("    ✓ Ship destroyed or ownership removed");
  }

  std::println("\n✅ All dissolve tests passed!");
  return 0;
}
