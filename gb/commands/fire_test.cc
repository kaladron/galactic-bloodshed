// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Create in-memory database
  TestContext ctx;

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create star system
  star_struct ss{};
  ss.star_id = 0;
  ss.pnames.push_back("TestPlanet");
  ss.AP[0] = 100;  // Player 1 APs
  StarRepository stars(store);
  stars.save(ss);

  // Create attacker ship - armed with guns
  ship_struct attacker{};
  attacker.number = 1;
  attacker.owner = 1;
  attacker.governor = 0;
  attacker.alive = true;
  attacker.on = true;
  attacker.type = ShipType::STYPE_BATTLE;
  attacker.guns = 1;  // Light guns
  attacker.destruct = 100;
  attacker.fuel = 1000.0;
  attacker.whatorbits = ScopeLevel::LEVEL_STAR;
  attacker.storbits = 0;
  attacker.xpos = 100.0;
  attacker.ypos = 200.0;
  attacker.mass = 100.0;
  attacker.build_cost = 100;

  auto attacker_handle = ctx.em.create_ship(attacker);
  attacker_handle.save();

  // Create target ship
  ship_struct target{};
  target.number = 2;
  target.owner = 2;
  target.governor = 0;
  target.alive = true;
  target.on = true;
  target.type = ShipType::STYPE_CARGO;
  target.whatorbits = ScopeLevel::LEVEL_STAR;
  target.storbits = 0;
  target.xpos = 110.0;
  target.ypos = 210.0;
  target.armor = 10;
  target.damage = 0;
  target.mass = 50.0;
  target.build_cost = 50;

  auto target_handle = ctx.em.create_ship(target);
  target_handle.save();

  // Create target race
  Race target_race{};
  target_race.Playernum = 2;
  target_race.Guest = false;
  target_race.governor[0].active = true;
  races.save(target_race);

  // Setup GameObj
  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Execute fire command
  command_t argv = {"fire", "#1", "#2", "10"};
  GB::commands::fire(argv, g);

  // Verify ships still exist in database (persisted via EntityManager)
  const auto* ship1 = ctx.em.peek_ship(1);
  assert(ship1);
  assert(ship1->number() == 1);

  const auto* ship2 = ctx.em.peek_ship(2);
  assert(ship2);
  assert(ship2->number() == 2);

  // Test passed - command executed and ships persisted via RAII
  std::println("fire_test.cc: All assertions passed!");
  return 0;
}
