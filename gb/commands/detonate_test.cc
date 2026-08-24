// SPDX-License-Identifier: Apache-2.0

/// \file detonate_test.cc
/// \brief Unit tests for detonate command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  // Create test context
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "MineLayer";
  race.Guest = false;
  race.Gov_ship = 0;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;
  race.tech = 100.0;
  race.morale = 100;

  RaceRepository races(store);
  races.save(race);

  // Create target race
  Race target_race{};
  target_race.Playernum = 2;
  target_race.name = "TargetRace";
  target_race.Guest = false;
  target_race.governor[0].active = true;
  target_race.tech = 100.0;
  races.save(target_race);

  // Create star with ship list pointing to mine (ship #1)
  star_struct star{};
  star.star_id = 0;
  star.name = "MineStar";
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
  mine.active = true;
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
  target.active = true;
  target.armor = 10;
  target.damage = 0;
  target.nextship = 0;  // End of ship list
  target.size = 20;     // Ship size for combat calculations
  target.popn = 10;
  target.tech = 10.0;

  auto target_handle = ctx.em.create_ship(target);
  target_handle.save();
}

void test_detonate_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Execute detonate command: detonate #1
  ctx.assert_dispatch_success(g, {"detonate", "#1"});

  std::println(std::cout, "Command output: {}", g.out.str());

  // Verify mine was detonated (destroyed)
  const auto* detonated_mine = ctx.em.peek_ship(1);

  // Mine should be destroyed after detonation
  if (detonated_mine) {
    test::expect_false(detonated_mine->alive());
  }

  // Target ship should be affected by the detonation
  const auto* affected_target = ctx.em.peek_ship(2);
  test::expect_ne(affected_target, nullptr);
  // Target should either be destroyed or damaged
  test::expect_true(!affected_target->alive() || affected_target->damage() > 0);

  std::println(std::cout,
               "✓ detonate command: Mine detonation persisted to database");
}

void test_detonate_role_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = 3;
  guest_race.name = "GuestMineLayer";
  guest_race.Guest = true;
  guest_race.governor[0].active = true;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"detonate", "#1"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
}

void test_detonate_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"detonate"});
  test::expect_contains(g.out.str(), "Syntax: detonate <mine>");

  // 2. Ship is not a mine
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"detonate", "#2"});

  // 3. Mine is not activated (on = false)
  {
    auto mine_handle = ctx.em.get_ship(1);
    mine_handle->on() = false;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"detonate", "#1"});
  test::expect_contains(g.out.str(), "not activated");
}

}  // namespace

int main() {
  test_detonate_happy_path();
  test_detonate_role_rejection();
  test_detonate_domain_errors();

  std::println(std::cout, "✓ detonate_test passed!");
  return 0;
}
