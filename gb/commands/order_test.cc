// SPDX-License-Identifier: Apache-2.0

/// \file order_test.cc
/// \brief Unit tests for order command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test ship with default orders - use battleship which can bombard
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_BATTLE;  // Battleship can bombard
  ship.name() = "TestShip";
  ship.speed() = 5;
  ship.max_speed() = 9;
  ship.max_crew() = 100;
  ship.popn() = 100;  // Crew

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);
}

void test_order_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "Set ship defense order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "on"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense order was set
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Defense order set: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 2: Turn defense order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "off"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense was turned off
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Defense order turned off: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 3: Set navigation order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "270", "4"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->navigate().on);
    test::expect_eq(saved_ship->navigate().bearing, 270U);
    test::expect_eq(saved_ship->navigate().turns, 4U);
    std::println(std::cout, "    ✓ Navigation order set: bearing=270, turns=4");
  }

  std::println(std::cout, "\nTest 4: Turn navigation order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "off"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->navigate().on);
    std::println(std::cout, "    ✓ Navigation order turned off");
  }

  std::println(std::cout, "\nTest 5: Set evasion order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "evade", "on"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().evade);
    std::println(std::cout, "    ✓ Evasion order turned on");
  }

  std::println(std::cout, "\nTest 6: Set retaliation order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "retaliate", "on"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().self);
    std::println(std::cout, "    ✓ Retaliation order turned on");
  }

  std::println(std::cout, "\nTest 7: Display all orders (no modifications)");
  {
    ctx.assert_dispatch_success(g, {"order"});

    // Verify ship state unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Display orders works without modification");
  }
}

}  // namespace

int main() {
  test_order_happy_path();

  std::println(std::cout, "All order tests passed!");
  return 0;
}
