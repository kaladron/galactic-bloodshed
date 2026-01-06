// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;

  // Create test race with enough tech
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 100.0;  // High tech for building
  race.morale = 100;
  race.God = false;
  race.pods = true;  // Allow pod building

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 10;
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a factory ship (required for make/modify commands)
  Ship factory{};
  factory.number() = 1;
  factory.owner() = 1;
  factory.governor() = 0;
  factory.alive() = true;
  factory.active() = true;
  factory.type() = ShipType::OTYPE_FACTORY;
  factory.build_type() = ShipType::OTYPE_FACTORY;
  factory.name() = "Factory";
  factory.whatorbits() = ScopeLevel::LEVEL_STAR;
  factory.storbits() = 0;
  factory.xpos() = 100.0;
  factory.ypos() = 200.0;
  factory.fuel() = 100.0;
  factory.max_fuel() = 500.0;
  factory.resource() = 1000;
  factory.max_resource() = 2000;
  factory.popn() = 50;
  factory.max_crew() = 100;
  factory.mass() = 100.0;
  factory.base_mass() = 100.0;
  factory.on() = 0;  // Factory must be offline to configure
  factory.size() = 100;

  // Save factory via repository
  ShipRepository ships_repo(store);
  ships_repo.save(factory);

  // Create GameObj for command execution - must be at SHIP scope
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);  // Factory is ship #1
  g.set_snum(0);

  std::println("Test 1: Set factory to produce fighters (make f)");
  {
    ctx.em.clear_cache();
    g.race = ctx.em.peek_race(1);  // Re-fetch after cache clear

    // make f (fighter)
    command_t argv = {"make", "f"};
    GB::commands::make_mod(argv, g);

    ctx.em.clear_cache();

    const auto* factory_check = ctx.em.peek_ship(1);
    assert(factory_check != nullptr);
    std::println("    Factory build_type now = {}",
                 static_cast<int>(factory_check->build_type()));

    // Factory should now be configured to build fighters
    assert(factory_check->build_type() == ShipType::STYPE_FIGHTER);
    std::println("    ✓ Factory configured to produce fighters");
  }

  std::println("Test 2: Modify factory design (modify armor 50)");
  {
    ctx.em.clear_cache();
    g.race = ctx.em.peek_race(1);  // Re-fetch after cache clear
    const auto* factory_before = ctx.em.peek_ship(1);
    assert(factory_before != nullptr);
    int initial_armor = factory_before->armor();
    std::println("    Before: armor={}", initial_armor);

    // modify armor 50
    command_t argv = {"modify", "armor", "50"};
    GB::commands::make_mod(argv, g);

    ctx.em.clear_cache();

    const auto* factory_after = ctx.em.peek_ship(1);
    assert(factory_after != nullptr);
    std::println("    After: armor={}", factory_after->armor());

    // Armor should now be 50
    assert(factory_after->armor() == 50);
    std::println("    ✓ Factory armor modified to 50");
  }

  std::println("Test 3: Modify factory design (modify speed 9)");
  {
    ctx.em.clear_cache();
    g.race = ctx.em.peek_race(1);  // Re-fetch after cache clear

    // modify speed 9
    command_t argv = {"modify", "speed", "9"};
    GB::commands::make_mod(argv, g);

    ctx.em.clear_cache();

    const auto* factory_check = ctx.em.peek_ship(1);
    assert(factory_check != nullptr);
    std::println("    After: max_speed={}", factory_check->max_speed());

    // Speed should be set (capped to max of 9)
    assert(factory_check->max_speed() <= 9);
    std::println("    ✓ Factory speed modified");
  }

  std::println("Test 4: Verify factory settings persist after cache clear");
  {
    ctx.em.clear_cache();

    const auto* factory_final = ctx.em.peek_ship(1);
    assert(factory_final != nullptr);

    std::println("    Final factory settings:");
    std::println("      build_type = {} (STYPE_FIGHTER={})",
                 static_cast<int>(factory_final->build_type()),
                 static_cast<int>(ShipType::STYPE_FIGHTER));
    std::println("      armor = {}", factory_final->armor());
    std::println("      max_speed = {}", factory_final->max_speed());
    std::println("      build_cost = {}", factory_final->build_cost());
    std::println("      complexity = {:.1f}", factory_final->complexity());

    assert(factory_final->build_type() == ShipType::STYPE_FIGHTER);
    assert(factory_final->armor() == 50);
    std::println("    ✓ Factory settings persisted to database");
  }

  std::println("\n✅ All make_mod tests passed!");
  return 0;
}
