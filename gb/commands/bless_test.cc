// SPDX-License-Identifier: Apache-2.0

/// \file bless_test.cc
/// \brief Unit tests for bless command

import dallib;
import dallib;
import gblib;
import commands;
import std;

#include <cassert>

// Test bless command - technology blessing
void test_bless_technology() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create a race via repository (simulating universe creation)
  Race race{};
  race.Playernum = 1;
  race.tech = 10.0;
  race.mass = 1.0;
  race.metabolism = 1.0;

  JsonStore store(db);
  RaceRepository race_repo(store);
  race_repo.save(race);

  // Create a minimal star and planet for bless to work at planet scope
  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.pnames.push_back("TestPlanet");
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Setup GameObj for command execution
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(true);  // Must be god to use bless

  // Execute bless command: bless 1 technology 5
  command_t argv = {"bless", "1", "technology", "5"};
  GB::commands::bless(argv, g);

  // Verify race technology was increased
  em.clear_cache();
  const auto* blessed_race = em.peek_race(1);
  assert(blessed_race);
  assert(blessed_race->tech == 15.0);  // 10 + 5

  std::println("✓ bless technology test passed");
}

// Test bless command - money blessing
void test_bless_money() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = 1;
  race.tech = 10.0;
  race.mass = 1.0;
  race.metabolism = 1.0;
  race.governor[0].money = 100;

  JsonStore store(db);
  RaceRepository race_repo(store);
  race_repo.save(race);

  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.pnames.push_back("TestPlanet");
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Setup GameObj for command execution
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(true);

  // Execute: bless 1 money 1000
  command_t argv = {"bless", "1", "money", "1000"};
  GB::commands::bless(argv, g);

  // Verify money was added
  em.clear_cache();
  const auto* blessed_race = em.peek_race(1);
  assert(blessed_race);
  assert(blessed_race->governor[0].money == 1100);  // 100 + 1000

  std::println("✓ bless money test passed");
}

// Test bless command - requires god privilege
void test_bless_requires_god() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = 1;
  race.tech = 10.0;
  race.mass = 1.0;
  race.metabolism = 1.0;

  JsonStore store(db);
  RaceRepository race_repo(store);
  race_repo.save(race);

  star_struct star{};
  star.star_id = 1;
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Setup GameObj without god privilege
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(false);  // NOT a god

  // Try to execute bless without god privilege
  command_t argv = {"bless", "1", "technology", "5"};
  GB::commands::bless(argv, g);

  // Verify tech unchanged and error message output
  em.clear_cache();
  const auto* race_ptr = em.peek_race(1);
  assert(race_ptr);
  assert(race_ptr->tech == 10.0);  // Unchanged

  std::string out_str = g.out.str();
  assert(out_str.find("not privileged") != std::string::npos);

  std::println("✓ bless requires god privilege test passed");
}

// Test bless command - requires planet scope
void test_bless_requires_planet_scope() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = 1;
  race.tech = 10.0;
  race.mass = 1.0;
  race.metabolism = 1.0;

  JsonStore store(db);
  RaceRepository race_repo(store);
  race_repo.save(race);

  // Setup GameObj at wrong scope
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.set_level(ScopeLevel::LEVEL_UNIV);  // Wrong scope!
  g.set_snum(0);
  g.set_pnum(0);
  g.set_god(true);

  // Try to execute bless at wrong scope
  command_t argv = {"bless", "1", "technology", "5"};
  GB::commands::bless(argv, g);

  // Verify tech unchanged and error message
  em.clear_cache();
  const auto* race_ptr = em.peek_race(1);
  assert(race_ptr);
  assert(race_ptr->tech == 10.0);  // Unchanged

  std::string out_str = g.out.str();
  assert(out_str.find("cs to the planet") != std::string::npos);

  std::println("✓ bless requires planet scope test passed");
}

int main() {
  test_bless_technology();
  test_bless_money();
  test_bless_requires_god();
  test_bless_requires_planet_scope();

  std::println("\n✅ All bless command tests passed!");
  return 0;
}
