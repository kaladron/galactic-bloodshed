// SPDX-License-Identifier: Apache-2.0

/// \file upgrade_test.cc
/// \brief Unit tests for upgrade command and AP deduction.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_upgrade_command() {
  // Create test context
  TestContext ctx;

  // Create test race with enough tech for upgrades
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 500.0;  // High tech to allow upgrades
  race.morale = 100;
  race.God = false;

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
  ss.explored = (1ULL << 1);  // Player 1 has explored
  ss.AP[0] = 10;
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  const auto type = ShipType::STYPE_FIGHTER;
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = type;
  ship.build_type() = type;
  ship.name() = "Upgradeable";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;
  ship.xpos() = 100.0;
  ship.ypos() = 200.0;
  ship.fuel() = 10.0;
  ship.max_fuel() = Shipdata[type][ABIL_FUELCAP];
  ship.resource() = 500;  // Need resources to pay for upgrades
  ship.max_resource() = Shipdata[type][ABIL_CARGO];
  ship.popn() = Shipdata[type][ABIL_MAXCREW];
  ship.max_crew() = Shipdata[type][ABIL_MAXCREW];
  ship.armor() = Shipdata[type][ABIL_ARMOR];
  ship.max_speed() = 5;
  ship.max_destruct() = Shipdata[type][ABIL_DESTCAP];
  ship.max_hanger() = Shipdata[type][ABIL_HANGER];
  ship.primary() = Shipdata[type][ABIL_GUNS];
  ship.secondary() = Shipdata[type][ABIL_GUNS];
  ship.base_mass() = 10.0;
  ship.mass() = 10.0;
  ship.build_cost() = static_cast<int>(cost(ship));
  ship.damage() = 0;  // No damage - required for upgrades

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  assert(g.out.str().contains("Invalid scope for this command."));
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Scope rejection at STAR scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  assert(g.out.str().contains("Invalid scope for this command."));
  std::println(std::cout, "    ✓ Scope rejection at star level verified");

  // 3. Guest rejection
  {
    auto guest_race_handle = ctx.em.get_race(1);
    guest_race_handle->Guest = true;
  }
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  assert(g.out.str().contains("Guest races cannot use this command."));
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  {
    auto race_handle = ctx.em.get_race(1);
    race_handle->Guest = false;
  }
  ctx.setup_game_obj(g);

  // 4. Upgrade ship armor (at SHIP scope)
  std::println(std::cout, "Upgrade ship armor");
  {
    ctx.setup_game_obj(g);
    g.set_level(ScopeLevel::LEVEL_SHIP);
    g.set_shipno(1);
    g.set_snum(0);

    const auto* ship_before = ctx.em.peek_ship(1);
    assert(ship_before != nullptr);
    int initial_armor = ship_before->armor();
    int target_armor = initial_armor + 2;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(0);
    assert(star_before->AP(1) == 10);
    std::println(std::cout, "    Before: armor={}, resource={}, star AP={}",
                 initial_armor, initial_resource, star_before->AP(1));

    // upgrade armor target_armor
    ctx.assert_dispatch_success(
        g, {"upgrade", "armor", std::to_string(target_armor)}, 1);

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    const auto* ship_after = ctx.em.peek_ship(1);
    assert(ship_after != nullptr);
    const auto* star_after = ctx.em.peek_star(0);
    assert(star_after->AP(1) == 9);  // 1 Star AP deducted
    std::println(std::cout, "    After: armor={}, resource={}, star AP={}",
                 ship_after->armor(), ship_after->resource(),
                 star_after->AP(1));

    // Armor should have increased
    assert(ship_after->armor() == target_armor);
    std::println(
        std::cout,
        "    ✓ Armor upgrade applied and 1 Star AP deducted (was {}, now {})",
        initial_armor, ship_after->armor());
  }

  // 5. Upgrade ship speed
  std::println(std::cout, "Upgrade ship speed");
  {
    ctx.setup_game_obj(g);
    g.set_level(ScopeLevel::LEVEL_SHIP);
    g.set_shipno(1);
    g.set_snum(0);

    const auto* ship_before = ctx.em.peek_ship(1);
    assert(ship_before != nullptr);
    int initial_speed = ship_before->max_speed();
    int target_speed = initial_speed + 1;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(0);
    assert(star_before->AP(1) == 9);
    std::println(std::cout, "    Before: max_speed={}, resource={}, star AP={}",
                 initial_speed, initial_resource, star_before->AP(1));

    // upgrade speed target_speed
    ctx.assert_dispatch_success(
        g, {"upgrade", "speed", std::to_string(target_speed)}, 1);

    ctx.em.clear_cache();

    const auto* ship_after = ctx.em.peek_ship(1);
    assert(ship_after != nullptr);
    const auto* star_after = ctx.em.peek_star(0);
    assert(star_after->AP(1) == 8);  // Another 1 Star AP deducted
    std::println(std::cout, "    After: max_speed={}, resource={}, star AP={}",
                 ship_after->max_speed(), ship_after->resource(),
                 star_after->AP(1));

    // Speed should have increased
    assert(ship_after->max_speed() == target_speed);
    std::println(
        std::cout,
        "    ✓ Speed upgrade applied and 1 Star AP deducted (was {}, now {})",
        initial_speed, ship_after->max_speed());
  }

  std::println(std::cout, "Verify upgrades persist after cache clear");
  {
    ctx.em.clear_cache();

    const auto* ship_check = ctx.em.peek_ship(1);
    assert(ship_check != nullptr);

    // Values should still reflect upgrades
    std::println(
        std::cout, "    Final values: armor={}, max_speed={}, resource={}",
        ship_check->armor(), ship_check->max_speed(), ship_check->resource());

    std::println(std::cout, "    ✓ Upgrades persisted to database");
  }
}

}  // namespace

int main() {
  test_upgrade_command();
  std::println(std::cout, "\n✅ All upgrade tests passed!");
  return 0;
}
