// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std;

#include <cassert>

// Test designating a capital ship
void test_designate_capital() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager entity_manager(db);
  JsonStore store(db);

  // Create race
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.Gov_ship = 0;

  RaceRepository races_repo(store);
  races_repo.save(race);

  // Create star
  star_struct star{};
  star.star_id = 1;
  star.name = "Test Star";
  star.AP[0] = 100;  // Sufficient AP for player 1

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create government ship landed on planet
  Ship ship{};
  ship.number() = 1;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.storbits() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ship.xpos() = 10.0;
  ship.ypos() = 10.0;
  ship.alive() = true;
  ship.active() = true;
  ship.docked() = true;  // Required for landed() check

  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Execute command
  GameObj g(entity_manager);
  g.player = 1;
  g.governor = 0;
  g.race = entity_manager.peek_race(1);  // Set race pointer like production

  command_t argv{"capital", "1"};
  GB::commands::capital(argv, g);

  // Verify race Gov_ship was updated through EntityManager
  const auto* updated_race = entity_manager.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->Gov_ship == 1);

  // Also verify it was persisted to database
  auto saved_race = races_repo.find_by_player(1);
  assert(saved_race.has_value());
  assert(saved_race->Gov_ship == 1);

  std::println("test_designate_capital passed!");
}

// Test non-leader attempting to designate capital
void test_governor_cannot_designate() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager entity_manager(db);
  JsonStore store(db);

  // Create race
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.Gov_ship = 0;

  RaceRepository races_repo(store);
  races_repo.save(race);

  // Execute command as governor (not leader)
  GameObj g(entity_manager);
  g.player = 1;
  g.governor = 1;                        // Not the leader
  g.race = entity_manager.peek_race(1);  // Set race pointer like production

  command_t argv{"capital", "1"};
  GB::commands::capital(argv, g);

  // Verify Gov_ship was NOT updated
  auto saved_race = races_repo.find_by_player(1);
  assert(saved_race.has_value());
  assert(saved_race->Gov_ship == 0);

  std::println("test_governor_cannot_designate passed!");
}

// Test ship not landed
void test_ship_not_landed() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager entity_manager(db);
  JsonStore store(db);

  // Create race
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.Gov_ship = 0;

  RaceRepository races_repo(store);
  races_repo.save(race);

  // Create star
  star_struct star{};
  star.star_id = 1;
  star.name = "Test Star";
  star.AP[0] = 100;

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create ship NOT landed (in orbit)
  Ship ship{};
  ship.number() = 1;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.storbits() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;  // Orbiting star, not landed
  ship.whatdest() = ScopeLevel::LEVEL_STAR;
  ship.alive() = true;
  ship.active() = true;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Execute command
  GameObj g(entity_manager);
  g.player = 1;
  g.governor = 0;
  g.race = entity_manager.peek_race(1);  // Set race pointer like production

  command_t argv{"capital", "1"};
  GB::commands::capital(argv, g);

  // Verify Gov_ship was NOT updated
  auto saved_race = races_repo.find_by_player(1);
  assert(saved_race.has_value());
  assert(saved_race->Gov_ship == 0);

  std::println("test_ship_not_landed passed!");
}

// Test querying current capital without changing
void test_query_capital() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager entity_manager(db);
  JsonStore store(db);

  // Create race with existing capital ship
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.Gov_ship = 42;

  RaceRepository races_repo(store);
  races_repo.save(race);

  // Create the capital ship
  Ship ship{};
  ship.number() = 42;
  ship.type() = ShipType::OTYPE_GOV;
  ship.owner() = 1;
  ship.name() = "Capital Ship";
  ship.alive() = true;
  ship.active() = true;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Execute command without argument (query mode)
  GameObj g(entity_manager);
  g.player = 1;
  g.governor = 0;
  g.race = entity_manager.peek_race(1);  // Set race pointer like production

  command_t argv{"capital"};  // No ship number argument
  GB::commands::capital(argv, g);

  // Note: Without capturing output, we just verify it doesn't crash
  // The actual ship info is displayed through g.out

  std::println("test_query_capital passed!");
}

int main() {
  test_designate_capital();
  test_governor_cannot_designate();
  test_ship_not_landed();
  test_query_capital();

  std::println("All capital_test tests passed!");
  return 0;
}
