// SPDX-License-Identifier: Apache-2.0

/// \file make_mod_test.cc
/// \brief Unit tests for make and modify commands for factory ship
/// configuration

import dallib;
import gblib;
import test;
import commands;
import std;

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

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Scope rejection at STAR scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"modify", "armor", "50"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
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
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  {
    auto race_handle = ctx.em.get_race(1);
    race_handle->Guest = false;
  }
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);  // Factory is ship #1
  g.set_snum(0);

  std::println(std::cout, "Set factory to produce fighters (make f)");
  {
    ctx.assert_dispatch_success(g, {"make", "f"});

    ctx.em.clear_cache();

    const auto* factory_check = ctx.em.peek_ship(1);
    test::expect_ne(factory_check, nullptr);
    std::println(std::cout, "    Factory build_type now = {}",
                 static_cast<int>(factory_check->build_type()));

    // Factory should now be configured to build fighters
    test::expect_eq(factory_check->build_type(), ShipType::STYPE_FIGHTER);
    std::println(std::cout, "    ✓ Factory configured to produce fighters");
  }

  std::println(std::cout, "Modify factory design (modify armor 50)");
  {
    const auto* factory_before = ctx.em.peek_ship(1);
    test::expect_ne(factory_before, nullptr);
    int initial_armor = factory_before->armor();
    std::println(std::cout, "    Before: armor={}", initial_armor);

    ctx.assert_dispatch_success(g, {"modify", "armor", "50"});

    ctx.em.clear_cache();

    const auto* factory_after = ctx.em.peek_ship(1);
    test::expect_ne(factory_after, nullptr);
    std::println(std::cout, "    After: armor={}", factory_after->armor());

    // Armor should now be 50
    test::expect_eq(factory_after->armor(), 50);
    std::println(std::cout, "    ✓ Factory armor modified to 50");
  }

  std::println(std::cout, "Modify factory design (modify speed 9)");
  {
    ctx.assert_dispatch_success(g, {"modify", "speed", "9"});

    ctx.em.clear_cache();

    const auto* factory_check = ctx.em.peek_ship(1);
    test::expect_ne(factory_check, nullptr);
    std::println(std::cout, "    After: max_speed={}",
                 factory_check->max_speed());

    // Speed should be set (capped to max of 9)
    test::expect_le(factory_check->max_speed(), 9);
    std::println(std::cout, "    ✓ Factory speed modified");
  }

  std::println(std::cout, "Verify factory settings persist after cache clear");
  {
    ctx.em.clear_cache();

    const auto* factory_final = ctx.em.peek_ship(1);
    test::expect_ne(factory_final, nullptr);

    std::println(std::cout, "    Final factory settings:");
    std::println(std::cout, "      build_type = {} (STYPE_FIGHTER={})",
                 static_cast<int>(factory_final->build_type()),
                 static_cast<int>(ShipType::STYPE_FIGHTER));
    std::println(std::cout, "      armor = {}", factory_final->armor());
    std::println(std::cout, "      max_speed = {}", factory_final->max_speed());
    std::println(std::cout, "      build_cost = {}",
                 factory_final->build_cost());
    std::println(std::cout, "      complexity = {:.1f}",
                 factory_final->complexity());

    test::expect_eq(factory_final->build_type(), ShipType::STYPE_FIGHTER);
    test::expect_eq(factory_final->armor(), 50);
    std::println(std::cout, "    ✓ Factory settings persisted to database");
  }

  std::println(std::cout, "\n✅ All make_mod tests passed!");
  return 0;
}
