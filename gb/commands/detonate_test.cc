// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.Guest = false;
  race.Gov_ship = 0;

  RaceRepository races(store);
  races.save(race);

  // Create star with ship list pointing to mine (ship #1)
  star_struct star{};
  star.star_id = 0;
  star.ships = 1;  // Head of ship list

  StarRepository stars(store);
  stars.save(star);

  // Create mine ship (activated)
  ship_struct mine{};
  mine.number = 1;
  mine.owner = 1;
  mine.governor = 0;
  mine.type = ShipType::STYPE_MINE;
  mine.xpos = 100.0;
  mine.ypos = 100.0;
  mine.whatorbits = ScopeLevel::LEVEL_STAR;
  mine.storbits = 0;
  mine.on = true;
  mine.alive = true;
  mine.docked = false;
  mine.destruct = 10;  // Mine charge
  mine.nextship = 2;   // Link to target ship
  mine.size = 10;      // Ship size for combat calculations
  mine.tech = 10.0;    // Tech level for range calculations

  auto mine_handle = ctx.em.create_ship(mine);
  mine_handle.save();

  // Create target ship nearby
  ship_struct target{};
  target.number = 2;
  target.owner = 2;
  target.governor = 0;
  target.type = ShipType::STYPE_CARGO;
  target.xpos = 105.0;  // Close to mine
  target.ypos = 105.0;
  target.whatorbits = ScopeLevel::LEVEL_STAR;
  target.storbits = 0;
  target.on = true;
  target.alive = true;
  target.armor = 10;
  target.damage = 0;
  target.nextship = 0;  // End of ship list
  target.size = 20;     // Ship size for combat calculations

  auto target_handle = ctx.em.create_ship(target);
  target_handle.save();

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Execute detonate command: detonate #1
  command_t argv = {"detonate", "#1", ""};
  GB::commands::detonate(argv, g);

  // Verify mine was detonated (destroyed)
  const auto* detonated_mine = ctx.em.peek_ship(1);

  // Mine should be destroyed after detonation
  if (detonated_mine) {
    assert(!detonated_mine->alive());
  }

  // Target ship should be affected by the detonation
  const auto* affected_target = ctx.em.peek_ship(2);
  assert(affected_target != nullptr);
  // Target should either be destroyed or damaged
  assert(!affected_target->alive() || affected_target->damage() > 0);

  std::println("✓ detonate command: Mine detonation persisted to database");
  return 0;
}
