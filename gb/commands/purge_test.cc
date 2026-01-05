// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create test race with God privileges
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = true;
  race.governor[0].active = true;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);

  // Test purge command
  command_t purge_argv = {"purge"};
  GB::commands::purge(purge_argv, g);

  std::string output = g.out.str();
  assert(output.find("Purged") != std::string::npos);

  std::println("✓ purge_test passed!");
  return 0;
}
